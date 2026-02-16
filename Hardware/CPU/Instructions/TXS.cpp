#include "TXS.h"

namespace Hardware::Instructions {

void TXS::Execute(CPU& cpu, Mem& mem) { 
    cpu.SP = 0x0100 | cpu.X; 
}

}  // namespace Hardware::Instructions
