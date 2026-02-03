#include "Mem.h"

void Mem::Init() {
    for (Byte& i : memoria) {
        i = 0;
    }
}

void Mem::WriteWord(Word dato, Word dir) {
    Write(dir, dato & 0xFF);
    Write(dir + 1, dato >> 8);
}

void Mem::Write(Word dir, Byte val) {
    if (writeHook) {
        writeHook(dir, val);
    }
    memoria[dir] = val;
}

void Mem::SetWriteHook(WriteHook hook) { writeHook = hook; }
