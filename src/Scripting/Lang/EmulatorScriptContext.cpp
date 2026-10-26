#include "EmulatorScriptContext.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace System::Hardware::Scripting::Lang {

std::optional<int64_t> EmulatorScriptContext::ResolveVariable(const std::string& name) const {
    if (m_cpu == nullptr) {
        return std::nullopt;
    }
    if (name == "PC" || name == "pc") {
        return static_cast<int64_t>(m_cpu->PC);
    }
    if (name == "A" || name == "a") {
        return static_cast<int64_t>(m_cpu->A);
    }
    if (name == "X" || name == "x") {
        return static_cast<int64_t>(m_cpu->X);
    }
    if (name == "Y" || name == "y") {
        return static_cast<int64_t>(m_cpu->Y);
    }
    if (name == "SP" || name == "sp") {
        return static_cast<int64_t>(m_cpu->SP);
    }
    if (name == "C" || name == "c") {
        return static_cast<int64_t>(m_cpu->C);
    }
    if (name == "Z" || name == "z") {
        return static_cast<int64_t>(m_cpu->Z);
    }
    if (name == "I" || name == "i") {
        return static_cast<int64_t>(m_cpu->I);
    }
    if (name == "D" || name == "d") {
        return static_cast<int64_t>(m_cpu->D);
    }
    if (name == "B" || name == "b") {
        return static_cast<int64_t>(m_cpu->B);
    }
    if (name == "V" || name == "v") {
        return static_cast<int64_t>(m_cpu->V);
    }
    if (name == "N" || name == "n") {
        return static_cast<int64_t>(m_cpu->N);
    }
    return std::nullopt;
}

bool EmulatorScriptContext::IsMemoryIdentifier(const std::string& name) {
    return name == "MEM" || name == "mem";
}

int64_t EmulatorScriptContext::ReadMemory(uint16_t address) const {
    if (m_bus != nullptr) {
        return static_cast<int64_t>(m_bus->ReadDirect(address));
    }
    return 0;
}

} // namespace System::Hardware::Scripting::Lang
