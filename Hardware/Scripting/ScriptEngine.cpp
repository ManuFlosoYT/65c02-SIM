#include "Hardware/Scripting/ScriptEngine.h"

#include <pocketpy.h>

#include <fstream>
#include <iostream>
#include <deque>
#include <mutex>

#include "Hardware/Core/Emulator.h"
#include "Hardware/Core/CartridgeLoader.h"

using namespace Hardware;

struct ScriptEngine::Impl {
    Core::Emulator& emulator;
    std::deque<std::string> outputQueue;
    std::mutex outputMutex;

    explicit Impl(Core::Emulator& emu) : emulator(emu) {}
    ~Impl() = default;
    
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
};

ScriptEngine::ScriptEngine(Core::Emulator& emulator)
    : emulatorRef(emulator), pImpl(std::make_unique<Impl>(emulator)) {}

ScriptEngine::~ScriptEngine() {
    Stop();
}

std::deque<std::string> ScriptEngine::GetOutput() const {
    std::lock_guard<std::mutex> lock(pImpl->outputMutex);
    return pImpl->outputQueue;
}

void ScriptEngine::ClearOutput() {
    std::lock_guard<std::mutex> lock(pImpl->outputMutex);
    pImpl->outputQueue.clear();
}

void ScriptEngine::AppendOutput(const std::string& text) {
    std::lock_guard<std::mutex> lock(pImpl->outputMutex);
    pImpl->outputQueue.emplace_back(text);
}

bool ScriptEngine::LoadAndRun(const std::string& filepath) {
    Stop();
    isRunning = true;
    workerThread = std::thread(&ScriptEngine::ScriptThread, this, filepath);
    return true;
}

void ScriptEngine::Stop() {
    isRunning = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

static bool py_emu_pause(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().Pause();
    py_newnone(py_retval());
    return true;
}

static bool py_emu_resume(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().Resume();
    py_newnone(py_retval());
    return true;
}

static void py_custom_print(const char* text) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    if (engine != nullptr) {
        engine->AppendOutput(text);
        if (engine->IsMirrorToStdoutEnabled()) {
            std::cout << text << std::flush;
        }
    } else {
        std::cout << text << std::flush;
    }
}

static bool py_emu_read_mem(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);           // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    auto addr = static_cast<uint16_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    uint8_t val = engine->GetEmulator().GetMem().Read(addr);
    py_newint(py_retval(), val);
    return true;
}

static bool py_emu_write_mem(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);           // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    PY_CHECK_ARG_TYPE(1, tp_int);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    auto addr = static_cast<uint16_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto val = static_cast<uint8_t>(py_toint(py_arg(1)));     // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetMem().Write(addr, val);
    py_newnone(py_retval());
    return true;
}

static bool py_emu_read_mem_block(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2); //NOLINT
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    PY_CHECK_ARG_TYPE(0, tp_int);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    PY_CHECK_ARG_TYPE(1, tp_int);
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto addr = static_cast<uint16_t>(py_toint(py_arg(0)));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto len = static_cast<size_t>(py_toint(py_arg(1)));

    unsigned char* buffer = py_newbytes(py_retval(), (int)len);
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetMem().ReadBlock(addr, std::span<uint8_t>(buffer, len));
    return true;
}

static bool py_emu_write_mem_block(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2); //NOLINT
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    PY_CHECK_ARG_TYPE(0, tp_int);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    PY_CHECK_ARG_TYPE(1, tp_bytes);
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto addr = static_cast<uint16_t>(py_toint(py_arg(0)));

    int size = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    unsigned char* buffer = py_tobytes(py_arg(1), &size);
    if (buffer == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetMem().WriteBlock(addr, std::span<const uint8_t>(buffer, static_cast<size_t>(size)));
    py_newnone(py_retval());
    return true;
}

static bool py_emu_load_bin(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2); //NOLINT
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    PY_CHECK_ARG_TYPE(0, tp_str);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    PY_CHECK_ARG_TYPE(1, tp_int);
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* path = py_tostr(py_arg(0));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto addr = static_cast<uint16_t>(py_toint(py_arg(1)));

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        py_newbool(py_retval(), false);
        return true;
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    for (size_t i = 0; i < buffer.size(); ++i) {
        engine->GetEmulator().GetMem().WriteDirect(static_cast<uint16_t>(addr + i), buffer[i]);
    }
    py_newbool(py_retval(), true);
    return true;
}

static bool py_emu_trigger_irq(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().TriggerIRQ();
    py_newnone(py_retval());
    return true;
}

static bool py_emu_trigger_nmi(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().TriggerNMI();
    py_newnone(py_retval());
    return true;
}

static bool py_emu_wait_cycles(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);           // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    auto cycles = static_cast<int64_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)

    if (engine->GetEmulator().IsPaused() || !engine->GetEmulator().IsRunning()) {
        for (int64_t i = 0; i < cycles; i++) {
            std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
            engine->GetEmulator().Step();
        }
    } else {
        uint64_t startCycles = engine->GetEmulator().GetTotalCycles();
        uint64_t targetCycles = startCycles + static_cast<uint64_t>(cycles);
        while (engine->GetEmulator().GetTotalCycles() < targetCycles) {
            std::this_thread::yield();
        }
    }
    
    py_newnone(py_retval());
    return true;
}

static bool py_emu_step(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().Step<true>();
    py_newnone(py_retval());
    return true;
}

static bool py_emu_step_instruction(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    auto& cpu = engine->GetEmulator().GetCPU();
    auto& bus = engine->GetEmulator().GetMem();
    cpu.Step<true>(bus);
    while (cpu.remainingCycles > 0) {
        cpu.Step<true>(bus);
    }
    py_newnone(py_retval());
    return true;
}

static bool py_emu_wait_instructions(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    auto count = static_cast<int>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    
    for (int i = 0; i < count; ++i) {
        std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
        auto& cpu = engine->GetEmulator().GetCPU();
        auto& bus = engine->GetEmulator().GetMem();
        cpu.Step<true>(bus);
        while (cpu.remainingCycles > 0) {
            cpu.Step<true>(bus);
        }
    }
    py_newnone(py_retval());
    return true;
}

static bool py_emu_reset(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().SetupHardware();
    py_newnone(py_retval());
    return true;
}

static bool py_emu_get_pc(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    py_newint(py_retval(), engine->GetEmulator().GetCPU().PC);
    return true;
}

static bool py_emu_set_pc(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetCPU().PC = static_cast<uint16_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    engine->GetEmulator().GetCPU().UpdatePagePtr(engine->GetEmulator().GetMem());
    py_newnone(py_retval());
    return true;
}

static bool py_emu_get_sp(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    py_newint(py_retval(), engine->GetEmulator().GetCPU().SP);
    return true;
}

static bool py_emu_set_sp(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetCPU().SP = static_cast<uint16_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_get_a(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    py_newint(py_retval(), engine->GetEmulator().GetCPU().A);
    return true;
}

static bool py_emu_set_a(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetCPU().A = static_cast<uint8_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_get_x(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    py_newint(py_retval(), engine->GetEmulator().GetCPU().X);
    return true;
}

static bool py_emu_set_x(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetCPU().X = static_cast<uint8_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_get_y(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    py_newint(py_retval(), engine->GetEmulator().GetCPU().Y);
    return true;
}

static bool py_emu_set_y(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetCPU().Y = static_cast<uint8_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_get_status(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    py_newint(py_retval(), engine->GetEmulator().GetCPU().GetStatus());
    return true;
}

static bool py_emu_set_status(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::lock_guard<std::mutex> lock(engine->GetEmulator().GetMutex());
    engine->GetEmulator().GetCPU().SetStatus(static_cast<uint8_t>(py_toint(py_arg(0))));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_set_gpu_enabled(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_bool);   // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().SetGPUEnabled(py_tobool(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_set_sd_enabled(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_bool);   // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().SetSDEnabled(py_tobool(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_set_esp_enabled(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_bool);   // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().SetESPEnabled(py_tobool(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_set_cycle_accurate(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_bool);   // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().SetCycleAccurate(py_tobool(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_get_cycles(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    py_newint(py_retval(), static_cast<int64_t>(engine->GetEmulator().GetTotalCycles()));
    return true;
}

static bool py_emu_get_ips(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    py_newint(py_retval(), engine->GetEmulator().GetActualIPS());
    return true;
}

static bool py_emu_set_target_ips(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().SetTargetIPS(static_cast<int>(py_toint(py_arg(0))));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_inject_key(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_str);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    const char* str = py_tostr(py_arg(0));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (str != nullptr && *str != '\0') {
        engine->GetEmulator().InjectKey(*str);
    }
    py_newnone(py_retval());
    return true;
}

static bool py_emu_start_audio_recording(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_str);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().GetSID().StartRecording(py_tostr(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_stop_audio_recording(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().GetSID().StopRecording();
    py_newnone(py_retval());
    return true;
}

static bool py_emu_load_cartridge(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_str);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    std::string path = py_tostr(py_arg(0));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::string error;
    Core::Cartridge cart;
    if (Core::CartridgeLoader::Load(path, cart, error)) {
        engine->GetEmulator().SetCartridge(cart);
        engine->GetEmulator().SetupHardware();
    } else {
        std::cerr << "Scripting API: Failed to load cartridge: " << error << "\n";
    }
    py_newnone(py_retval());
    return true;
}

static void ParseBreakCondition(py_Ref cond_dict, Hardware::BreakCondition& cond) {
    if (py_dict_getitem_by_str(cond_dict, "type") == 1) {
        cond.type = static_cast<Hardware::BreakpointType>(py_toint(py_retval()));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    }
    if (py_dict_getitem_by_str(cond_dict, "reg") == 1) {
        cond.reg = static_cast<Hardware::RegisterId>(py_toint(py_retval()));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    }
    if (py_dict_getitem_by_str(cond_dict, "flag") == 1) {
        cond.flag = static_cast<Hardware::FlagId>(py_toint(py_retval()));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    }
    if (py_dict_getitem_by_str(cond_dict, "op") == 1) {
        cond.op = static_cast<Hardware::CompareOp>(py_toint(py_retval()));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    }
    if (py_dict_getitem_by_str(cond_dict, "address") == 1) {
        cond.address = static_cast<uint16_t>(py_toint(py_retval()));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    }
    if (py_dict_getitem_by_str(cond_dict, "value") == 1) {
        cond.value = static_cast<uint16_t>(py_toint(py_retval()));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    }
}

static bool py_emu_add_breakpoint(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_dict);   // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    py_Ref dict = py_arg(0);         // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)

    Hardware::Breakpoint breakpoint;
    if (py_dict_getitem_by_str(dict, "label") == 1) {
        breakpoint.label = py_tostr(py_retval());
    }
    if (py_dict_getitem_by_str(dict, "enabled") == 1) {
        breakpoint.enabled = py_tobool(py_retval());
    }
    if (py_dict_getitem_by_str(dict, "compoundOp") == 1) {
        breakpoint.compoundOp = static_cast<Hardware::LogicOp>(py_toint(py_retval()));
    }

    if (py_dict_getitem_by_str(dict, "conditions") == 1) {
        py_assign(py_r1(), py_retval());
        int len = py_list_len(py_r1());
        for (int i = 0; i < len; ++i) {
            py_Ref cond_dict = py_list_getitem(py_r1(), i);
            if (py_isdict(cond_dict)) {
                Hardware::BreakCondition cond;
                ParseBreakCondition(cond_dict, cond);
                breakpoint.conditions.push_back(cond);
            }
        }
    }

    uint32_t bpid = engine->GetEmulator().GetBreakpointManager().AddBreakpoint(breakpoint);
    py_newint(py_retval(), bpid);
    return true;
}

static bool py_emu_remove_breakpoint(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);             // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().GetBreakpointManager().RemoveBreakpoint(static_cast<uint32_t>(py_toint(py_arg(0))));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    py_newnone(py_retval());
    return true;
}

static bool py_emu_clear_breakpoints(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().GetBreakpointManager().ClearAll();
    py_newnone(py_retval());
    return true;
}

static void SerializeBreakCondition(py_Ref out_dict, const Hardware::BreakCondition& cond) {
    py_newint(py_r0(), static_cast<int64_t>(cond.type));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_dict_setitem_by_str(out_dict, "type", py_r0());  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_newint(py_r0(), static_cast<int64_t>(cond.reg));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_dict_setitem_by_str(out_dict, "reg", py_r0());  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_newint(py_r0(), static_cast<int64_t>(cond.flag));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_dict_setitem_by_str(out_dict, "flag", py_r0());  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_newint(py_r0(), static_cast<int64_t>(cond.op));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_dict_setitem_by_str(out_dict, "op", py_r0());  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_newint(py_r0(), static_cast<int64_t>(cond.address));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_dict_setitem_by_str(out_dict, "address", py_r0());  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_newint(py_r0(), static_cast<int64_t>(cond.value));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
    py_dict_setitem_by_str(out_dict, "value", py_r0());  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-cstyle-cast)
}

static bool py_emu_list_breakpoints(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    const auto& breakpoints = engine->GetEmulator().GetBreakpointManager().GetBreakpoints();
    
    py_newlist(py_retval());
    py_assign(py_r2(), py_retval()); // Keep the outer list in r2

    for (const auto& breakpoint : breakpoints) {
        py_newdict(py_r3()); // Create dict for this BP
        
        py_newint(py_r0(), static_cast<int64_t>(breakpoint.id));
        py_dict_setitem_by_str(py_r3(), "id", py_r0());
        
        py_newstr(py_r0(), breakpoint.label.c_str());
        py_dict_setitem_by_str(py_r3(), "label", py_r0());
        
        py_newbool(py_r0(), breakpoint.enabled);
        py_dict_setitem_by_str(py_r3(), "enabled", py_r0());
        
        py_newint(py_r0(), static_cast<int64_t>(breakpoint.compoundOp));
        py_dict_setitem_by_str(py_r3(), "compoundOp", py_r0());

        // conditions list
        py_newlist(py_r4()); 
        for (const auto& cond : breakpoint.conditions) {
            py_newdict(py_r5());
            SerializeBreakCondition(py_r5(), cond);
            py_list_append(py_r4(), py_r5());
        }
        py_dict_setitem_by_str(py_r3(), "conditions", py_r4());
        
        py_list_append(py_r2(), py_r3());
    }
    py_assign(py_retval(), py_r2());
    return true;
}

void ScriptEngine::ScriptThread(const std::string& filepath) {
    py_initialize();
    py_setvmctx(this);
    py_callbacks()->print = py_custom_print;
    
    py_GlobalRef mod = py_newmodule("emu");
    py_bindfunc(mod, "pause", py_emu_pause);
    py_bindfunc(mod, "resume", py_emu_resume);
    py_bindfunc(mod, "step", py_emu_step);
    py_bindfunc(mod, "step_instruction", py_emu_step_instruction);
    py_bindfunc(mod, "wait_instructions", py_emu_wait_instructions);
    py_bindfunc(mod, "reset", py_emu_reset);
    py_bindfunc(mod, "read_mem", py_emu_read_mem);
    py_bindfunc(mod, "write_mem", py_emu_write_mem);
    py_bindfunc(mod, "read_mem_block", py_emu_read_mem_block);
    py_bindfunc(mod, "write_mem_block", py_emu_write_mem_block);
    py_bindfunc(mod, "load_bin", py_emu_load_bin);
    py_bindfunc(mod, "wait_cycles", py_emu_wait_cycles);
    
    py_bindfunc(mod, "get_pc", py_emu_get_pc);
    py_bindfunc(mod, "set_pc", py_emu_set_pc);
    py_bindfunc(mod, "get_sp", py_emu_get_sp);
    py_bindfunc(mod, "set_sp", py_emu_set_sp);
    py_bindfunc(mod, "get_a", py_emu_get_a);
    py_bindfunc(mod, "set_a", py_emu_set_a);
    py_bindfunc(mod, "get_x", py_emu_get_x);
    py_bindfunc(mod, "set_x", py_emu_set_x);
    py_bindfunc(mod, "get_y", py_emu_get_y);
    py_bindfunc(mod, "set_y", py_emu_set_y);
    py_bindfunc(mod, "get_status", py_emu_get_status);
    py_bindfunc(mod, "set_status", py_emu_set_status);
    
    py_bindfunc(mod, "trigger_irq", py_emu_trigger_irq);
    py_bindfunc(mod, "trigger_nmi", py_emu_trigger_nmi);
    
    py_bindfunc(mod, "set_gpu_enabled", py_emu_set_gpu_enabled);
    py_bindfunc(mod, "set_sd_enabled", py_emu_set_sd_enabled);
    py_bindfunc(mod, "set_esp_enabled", py_emu_set_esp_enabled);
    py_bindfunc(mod, "set_cycle_accurate", py_emu_set_cycle_accurate);
    
    py_bindfunc(mod, "get_cycles", py_emu_get_cycles);
    py_bindfunc(mod, "get_ips", py_emu_get_ips);
    py_bindfunc(mod, "set_target_ips", py_emu_set_target_ips);
    py_bindfunc(mod, "inject_key", py_emu_inject_key);
    py_bindfunc(mod, "set_key_pressed", py_emu_inject_key);
    
    py_bindfunc(mod, "start_audio_recording", py_emu_start_audio_recording);
    py_bindfunc(mod, "stop_audio_recording", py_emu_stop_audio_recording);
    py_bindfunc(mod, "load_cartridge", py_emu_load_cartridge);
    py_bindfunc(mod, "add_breakpoint", py_emu_add_breakpoint);
    py_bindfunc(mod, "remove_breakpoint", py_emu_remove_breakpoint);
    py_bindfunc(mod, "clear_breakpoints", py_emu_clear_breakpoints);
    py_bindfunc(mod, "list_breakpoints", py_emu_list_breakpoints);

    auto set_const = [&](const char* name, int val) {
        py_newint(py_r0(), static_cast<int64_t>(val));
        py_setdict(mod, py_name(name), py_r0());
    };

    set_const("BP_TYPE_PC", 0);
    set_const("BP_TYPE_REG", 1);
    set_const("BP_TYPE_FLAG", 2);
    set_const("BP_TYPE_MEM", 3);
    set_const("BP_TYPE_WATCH", 4);

    set_const("REG_A", 0);
    set_const("REG_X", 1);
    set_const("REG_Y", 2);
    set_const("REG_SP", 3);
    set_const("REG_PC", 4);

    set_const("OP_EQUAL", 0);
    set_const("OP_NOT_EQUAL", 1);
    set_const("OP_LESS", 2);
    set_const("OP_LESS_EQUAL", 3);
    set_const("OP_GREATER", 4);
    set_const("OP_GREATER_EQUAL", 5);

    set_const("LOGIC_AND", 0);
    set_const("LOGIC_OR", 1);

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ScriptEngine: Failed to open script " << filepath << '\n';
        isRunning = false;
        py_finalize();
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!py_exec(content.c_str(), filepath.c_str(), EXEC_MODE, nullptr)) {
        py_printexc();
    }
    
    py_finalize();
    isRunning = false;
}
