#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace Hardware {
class CPU;
class Bus;
}

namespace System::Hardware::Scripting::Lang {

class EmulatorScriptContext {
public:
    EmulatorScriptContext() = default;
    EmulatorScriptContext(const ::Hardware::CPU* cpu, const ::Hardware::Bus* bus)
        : m_cpu(cpu), m_bus(bus) {}

    void SetContext(const ::Hardware::CPU* cpu, const ::Hardware::Bus* bus) {
        m_cpu = cpu;
        m_bus = bus;
    }

    const ::Hardware::CPU* GetCPU() const { return m_cpu; }
    const ::Hardware::Bus* GetBus() const { return m_bus; }

    std::optional<int64_t> ResolveVariable(const std::string& name) const;
    static bool IsMemoryIdentifier(const std::string& name);
    int64_t ReadMemory(uint16_t address) const;

private:
    const ::Hardware::CPU* m_cpu = nullptr;
    const ::Hardware::Bus* m_bus = nullptr;
};

} // namespace System::Hardware::Scripting::Lang
