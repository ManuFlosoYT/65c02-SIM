#include "Execution.h"

#include "../CPU.h"
#include "Dispatch.h"
#include "MemoryOps.h"
#include "StackOps.h"
#include "StatusOps.h"

namespace Hardware {
namespace CPUExecution {

int Execute(CPU& cpu, Mem& mem) {
    while (true) {
        int res = Step(cpu, mem);
        if (res != 0) return res;
    }
    return 0;
}

void IRQ(CPU& cpu, Mem& mem) {
    cpu.waiting = false;
    if (!cpu.I) {
        CPUStackOps::PushWord(cpu, cpu.PC, mem);
        cpu.B = 0;
        CPUStackOps::PushByte(cpu, CPUStatusOps::GetStatus(cpu), mem);
        cpu.I = 1;
        cpu.D = 0;  // 65C02 clears Decimal flag in IRQ
        cpu.PC = CPUMemoryOps::ReadWord(0xFFFE, mem);
    }
}

int Step(CPU& cpu, Mem& mem) {
    if (!cpu.isInit) {
        cpu.PC = CPUMemoryOps::ReadWord(0xFFFC, mem);
        cpu.isInit = true;
    }

    // Check interrupts
    if (cpu.waiting) {
        if ((mem.Read(ACIA_STATUS) & 0x80) != 0) {
            cpu.waiting = false;
        } else {
            return 0;
        }
    }

    // In cycle-accurate mode, consume remaining cycles
    if (cpu.cycleAccurate && cpu.remainingCycles > 0) {
        cpu.remainingCycles--;
        return 0;
    }

    return CPUDispatch::Dispatch(cpu, mem);
}

void Reset(CPU& cpu, Mem& mem) {
    cpu.PC = 0xFFFC;
    cpu.SP = 0x01FF;  // Top of Stack

    // Reset registers
    cpu.A = 0;
    cpu.X = 0;
    cpu.Y = 0;

    // Reset flags
    cpu.C = 0;
    cpu.Z = 0;
    cpu.I = 0;
    cpu.D = 0;
    cpu.B = 0;
    cpu.V = 0;
    cpu.N = 0;
    cpu.isInit = false;
}

}  // namespace CPUExecution
}  // namespace Hardware
