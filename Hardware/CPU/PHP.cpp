#include "PHP.h"

void PHP::Execute(CPU& cpu, Mem& mem) {    
    cpu.PushByte(cpu.GetStatus(), mem);
}
