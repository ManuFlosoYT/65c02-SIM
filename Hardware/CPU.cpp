#include "CPU.h"

#include "CPU/Dispatch.h"
#include "CPU/Execution.h"
#include "CPU/MemoryOps.h"
#include "CPU/StackOps.h"

namespace Hardware {

int CPU::Execute(Mem& mem) { return CPUExecution::Execute(*this, mem); }

void CPU::IRQ(Mem& mem) { CPUExecution::IRQ(*this, mem); }

int CPU::Step(Mem& mem) { return CPUExecution::Step(*this, mem); }

void CPU::Reset(Mem& mem) { CPUExecution::Reset(*this, mem); }

int CPU::Dispatch(Mem& mem) { return CPUDispatch::Dispatch(*this, mem); }

void CPU::PushByte(Byte val, Mem& mem) {
    CPUStackOps::PushByte(*this, val, mem);
}

Byte CPU::PopByte(Mem& mem) { return CPUStackOps::PopByte(*this, mem); }

void CPU::PushWord(Word val, Mem& mem) {
    CPUStackOps::PushWord(*this, val, mem);
}

Word CPU::PopWord(Mem& mem) { return CPUStackOps::PopWord(*this, mem); }

}  // namespace Hardware
