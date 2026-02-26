#include "Hardware/CPU/Dispatch.h"
#include "Hardware/CPU/DispatchTable.inl"

namespace Hardware {
namespace CPUDispatch {

inline int Dispatch(CPU& cpu, Bus& bus) {
    Byte opcode = cpu.FetchByte(bus);
    const OpcodeEntry& entry = dispatchTable[opcode];

    // Execute instruction
    if (entry.executor) {
        entry.executor(cpu, bus);
    }

    // Assign cycle accuracy offsets
    if (cpu.cycleAccurate && entry.baseCycles > 0) {
        cpu.remainingCycles = entry.baseCycles - 1;
    }

    // Error handling
    if (entry.exitCode == -1) {
        std::cerr << "Unknown opcode: 0x" << std::hex
                  << static_cast<int>(opcode) << " PC: 0x" << cpu.PC << std::dec
                  << " execution cancelled." << std::endl;
    }

    return entry.exitCode;
}

}  // namespace CPUDispatch
}  // namespace Hardware
