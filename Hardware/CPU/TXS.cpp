#include "TXS.h"

void TXS::Execute(CPU& cpu, Mem& mem) { 
    cpu.SP = 0x0100 | cpu.X; 
}
