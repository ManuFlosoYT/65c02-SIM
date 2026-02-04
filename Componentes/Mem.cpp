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
    auto it = writeHooks.find(dir);
    if (it != writeHooks.end()) {
        it->second(dir, val);
    }
    memoria[dir] = val;
}

void Mem::SetWriteHook(Word address, WriteHook hook) {
    writeHooks[address] = hook;
}

void Mem::SetReadHook(Word address, ReadHook hook) {
    readHooks[address] = hook;
}

Byte Mem::Read(Word dir) {
    auto it = readHooks.find(dir);
    if (it != readHooks.end()) {
        return it->second(dir);
    }
    return memoria[dir];
}
