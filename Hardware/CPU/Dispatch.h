#pragma once
#include "../Mem.h"

namespace Hardware {

class CPU;

namespace CPUDispatch {

int Dispatch(CPU& cpu, Mem& mem);

}  // namespace CPUDispatch

}  // namespace Hardware
