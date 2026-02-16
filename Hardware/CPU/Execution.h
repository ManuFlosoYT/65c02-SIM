#pragma once
#include "../Mem.h"

namespace Hardware {

class CPU;

namespace CPUExecution {

int Execute(CPU& cpu, Mem& mem);
int Step(CPU& cpu, Mem& mem);
void IRQ(CPU& cpu, Mem& mem);
void Reset(CPU& cpu, Mem& mem);

}  // namespace CPUExecution

}  // namespace Hardware
