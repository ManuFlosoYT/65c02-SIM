#include "TXS.h"

void TXS::Ejecutar(CPU& cpu, Mem& mem) { 
    cpu.SP = 0x0100 | cpu.X; 
}
