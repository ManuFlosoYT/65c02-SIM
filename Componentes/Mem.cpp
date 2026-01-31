#include "Mem.h"

void Mem::Init() {
    for (Byte& i : memoria) {
        i = 0;
    }
}

void Mem::WriteWord(Word dato, Word dir){
    memoria[dir]        = dato & 0xFF; // 8 bits menos significativos
    memoria[dir + 1]    = (dato >> 8); // 8 bits mas significativos
}
