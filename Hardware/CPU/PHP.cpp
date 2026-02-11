#include "PHP.h"

void PHP::Ejecutar(CPU& cpu, Mem& mem) {    
    cpu.PushByte(cpu.GetStatus(), mem);
}
