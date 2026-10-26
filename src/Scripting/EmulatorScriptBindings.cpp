#include "Scripting/EmulatorScriptBindings.h"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "Hardware/Core/Emulator/Emulator.h"
#include "Scripting/Lang/Interpreter.h"

namespace Hardware {

using namespace System::Hardware::Scripting::Lang;

void EmulatorScriptBindings::Register(
    Interpreter& interp,
    Core::Emulator& emu,
    std::function<bool()> abortCheck
) {
    auto isAborted = [check = std::move(abortCheck)]() -> bool {
        return check && check();
    };

    interp.defineBuiltin("emu.pause", [&emu](const std::vector<Value>&) -> Value {
        emu.Pause();
        return std::monostate{};
    });

    interp.defineBuiltin("emu.resume", [&emu](const std::vector<Value>&) -> Value {
        emu.Resume();
        return std::monostate{};
    });

    interp.defineBuiltin("emu.reset", [&emu](const std::vector<Value>&) -> Value {
        emu.Reset();
        auto& cpu = emu.GetCPU();
        auto& bus = emu.GetMem();
        uint16_t low = bus.Read(0xFFFC);
        uint16_t high = bus.Read(0xFFFD);
        cpu.PC = static_cast<uint16_t>(low | (high << 8));
        cpu.UpdatePagePtr(bus);
        return std::monostate{};
    });

    interp.defineBuiltin("emu.step", [&emu, isAborted](const std::vector<Value>&) -> Value {
        if (isAborted()) throw std::runtime_error("Script aborted");
        emu.Step<true>();
        while (emu.GetCPU().remainingCycles > 0) {
            if (isAborted()) throw std::runtime_error("Script aborted");
            emu.Step<true>();
        }
        return std::monostate{};
    });

    interp.defineBuiltin("emu.wait_cycles", [&emu, isAborted](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        int64_t cycles = std::get<int64_t>(args[0]);
        if (cycles <= 0) return std::monostate{};

        uint64_t targetCycles = emu.GetTotalCycles() + static_cast<uint64_t>(cycles);

        while (emu.GetTotalCycles() < targetCycles) {
            if (isAborted() || emu.IsHalted()) throw std::runtime_error("Script aborted or Emulator halted");
            emu.Step();
        }
        return std::monostate{};
    });

    interp.defineBuiltin("emu.wait_instructions", [&emu, isAborted](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        int64_t insts = std::get<int64_t>(args[0]);
        if (insts <= 0) return std::monostate{};

        uint64_t targetInsts = emu.GetTotalInstructions() + static_cast<uint64_t>(insts);

        while (emu.GetTotalInstructions() < targetInsts) {
            if (isAborted() || emu.IsHalted()) throw std::runtime_error("Script aborted or Emulator halted");
            emu.Step();
        }
        return std::monostate{};
    });

    interp.defineBuiltin("emu.write_mem", [&emu](const std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<int64_t>(args[0]) || !std::holds_alternative<int64_t>(args[1])) return std::monostate{};
        auto addr = static_cast<uint16_t>(std::get<int64_t>(args[0]));
        auto val = static_cast<uint8_t>(std::get<int64_t>(args[1]));
        emu.GetMem().Write(addr, val);
        return std::monostate{};
    });

    interp.defineBuiltin("emu.read_mem", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        auto addr = static_cast<uint16_t>(std::get<int64_t>(args[0]));
        return static_cast<int64_t>(emu.GetMem().Read(addr));
    });

    interp.defineBuiltin("emu.get_pc", [&emu](const std::vector<Value>&) -> Value {
        return static_cast<int64_t>(emu.GetCPU().PC);
    });

    interp.defineBuiltin("emu.set_pc", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        emu.GetCPU().PC = static_cast<uint16_t>(std::get<int64_t>(args[0]));
        emu.GetCPU().UpdatePagePtr(emu.GetMem());
        return std::monostate{};
    });

    interp.defineBuiltin("emu.write_mem_direct", [&emu](const std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<int64_t>(args[0]) || !std::holds_alternative<int64_t>(args[1])) return std::monostate{};
        auto addr = static_cast<uint16_t>(std::get<int64_t>(args[0]));
        auto val = static_cast<uint8_t>(std::get<int64_t>(args[1]));
        emu.WaitUntilSafeToMutate();
        emu.GetMem().WriteDirect(addr, val);
        return std::monostate{};
    });

    interp.defineBuiltin("emu.clear_breakpoints", [&emu](const std::vector<Value>&) -> Value {
        emu.GetBreakpointManager().ClearAll();
        return std::monostate{};
    });

    interp.defineBuiltin("emu.set_gpu_enabled", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<bool>(args[0])) return std::monostate{};
        emu.SetGPUEnabled(std::get<bool>(args[0]));
        return std::monostate{};
    });

    interp.defineBuiltin("emu.set_cycle_accurate", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<bool>(args[0])) return std::monostate{};
        emu.SetCycleAccurate(std::get<bool>(args[0]));
        return std::monostate{};
    });

    interp.defineBuiltin("emu.set_sd_enabled", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<bool>(args[0])) return std::monostate{};
        emu.SetSDEnabled(std::get<bool>(args[0]));
        return std::monostate{};
    });

    interp.defineBuiltin("emu.set_key_pressed", [](const std::vector<Value>&) -> Value {
        return std::monostate{};
    });

    interp.defineBuiltin("emu.write_mem_block", [&emu](const std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<int64_t>(args[0]) || !std::holds_alternative<std::string>(args[1])) return std::monostate{};
        auto addr = static_cast<uint16_t>(std::get<int64_t>(args[0]));
        const auto& str = std::get<std::string>(args[1]);

        for (char c : str) {
            emu.GetMem().Write(addr++, static_cast<uint8_t>(c));
        }
        return std::monostate{};
    });

    interp.defineBuiltin("emu.write_mem_block_direct", [&emu](const std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<int64_t>(args[0]) || !std::holds_alternative<std::string>(args[1])) return std::monostate{};
        auto addr = static_cast<uint16_t>(std::get<int64_t>(args[0]));
        const auto& str = std::get<std::string>(args[1]);

        emu.WaitUntilSafeToMutate();
        for (char c : str) {
            emu.GetMem().WriteDirect(addr++, static_cast<uint8_t>(c));
        }
        return std::monostate{};
    });

    interp.defineBuiltin("emu.read_mem_block", [&emu](const std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<int64_t>(args[0]) || !std::holds_alternative<int64_t>(args[1])) return std::monostate{};
        auto addr = static_cast<uint16_t>(std::get<int64_t>(args[0]));
        auto size = static_cast<uint16_t>(std::get<int64_t>(args[1]));

        std::string res;
        res.reserve(size);
        for (int i = 0; i < size; ++i) res.push_back(static_cast<char>(emu.GetMem().Read(addr++)));
        return res;
    });

    interp.defineBuiltin("emu.load_bin", [&emu](const std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<std::string>(args[0]) || !std::holds_alternative<int64_t>(args[1])) return false;

        std::ifstream file(std::get<std::string>(args[0]), std::ios::binary | std::ios::ate);
        if (!file) return false;
        auto sz = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> buf(sz);
        file.read(reinterpret_cast<char*>(buf.data()), sz);

        emu.WaitUntilSafeToMutate();
        auto addr = static_cast<uint16_t>(std::get<int64_t>(args[1]));
        for (auto b : buf) emu.GetMem().WriteDirect(addr++, b);
        return true;
    });

    interp.defineBuiltin("emu.add_breakpoint", [](const std::vector<Value>&) -> Value {
        return static_cast<int64_t>(1);
    });

    interp.defineBuiltin("emu.get_a", [&emu](const std::vector<Value>&) -> Value {
        return static_cast<int64_t>(emu.GetCPU().A);
    });

    interp.defineBuiltin("emu.set_a", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        emu.GetCPU().A = static_cast<uint8_t>(std::get<int64_t>(args[0]));
        return std::monostate{};
    });

    interp.defineBuiltin("emu.get_x", [&emu](const std::vector<Value>&) -> Value {
        return static_cast<int64_t>(emu.GetCPU().X);
    });

    interp.defineBuiltin("emu.set_x", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        emu.GetCPU().X = static_cast<uint8_t>(std::get<int64_t>(args[0]));
        return std::monostate{};
    });

    interp.defineBuiltin("emu.get_y", [&emu](const std::vector<Value>&) -> Value {
        return static_cast<int64_t>(emu.GetCPU().Y);
    });

    interp.defineBuiltin("emu.set_y", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        emu.GetCPU().Y = static_cast<uint8_t>(std::get<int64_t>(args[0]));
        return std::monostate{};
    });

    interp.defineBuiltin("emu.get_sp", [&emu](const std::vector<Value>&) -> Value {
        return static_cast<int64_t>(emu.GetCPU().SP);
    });

    interp.defineBuiltin("emu.set_sp", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        emu.GetCPU().SP = static_cast<uint16_t>(std::get<int64_t>(args[0]));
        return std::monostate{};
    });

    interp.defineBuiltin("emu.get_status", [&emu](const std::vector<Value>&) -> Value {
        return static_cast<int64_t>(emu.GetCPU().GetStatus());
    });

    interp.defineBuiltin("emu.set_status", [&emu](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0])) return std::monostate{};
        emu.GetCPU().SetStatus(static_cast<uint8_t>(std::get<int64_t>(args[0])));
        return std::monostate{};
    });

    interp.defineBuiltin("emu.trigger_irq", [&emu](const std::vector<Value>&) -> Value {
        emu.TriggerIRQ();
        return std::monostate{};
    });

    interp.defineBuiltin("emu.trigger_nmi", [&emu](const std::vector<Value>&) -> Value {
        emu.TriggerNMI();
        return std::monostate{};
    });
}

} // namespace Hardware
