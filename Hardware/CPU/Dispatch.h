#pragma once
#include "Hardware/Core/Bus.h"

namespace Hardware {

class CPU;

namespace CPUDispatch {

int Dispatch(CPU& cpu, Bus& bus);

}  // namespace CPUDispatch

}  // namespace Hardware
