#include "TXS.h"

namespace Hardware::Instructions {

void TXS::Execute(CPU& cpu, Bus& bus) { 
    cpu.SP = 0x0100 | cpu.X; 
}

}  // namespace Hardware::Instructions
