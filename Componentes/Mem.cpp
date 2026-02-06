#include "Mem.h"

void Mem::Init() {
    for (Byte& byte : memoria) {
        byte = 0;
    }
}

void Mem::WriteWord(Word dato, Word dir) {
    Write(dir, dato & 0xFF);    // Escribir el byte menos significativo
    Write(dir + 1, dato >> 8);  // Escribir el byte más significativo
}

void Mem::Write(Word dir, Byte val) {
    auto iterador = writeHooks.find(dir);   // Buscar el hook
    if (iterador != writeHooks.end()) {
        iterador->second(dir, val);        // Ejecutar el hook
    }
    memoria[dir] = val;
}

void Mem::SetWriteHook(Word address, WriteHook hook) {
    writeHooks[address] = hook;    // Asignar el hook
}

void Mem::SetReadHook(Word address, ReadHook hook) {
    readHooks[address] = hook;     // Asignar el hook
}

Byte Mem::Read(Word dir) {
    auto iterador = readHooks.find(dir);    // Buscar el hook
    if (iterador != readHooks.end()) {
        return iterador->second(dir);       // Ejecutar el hook
    }
    return memoria[dir];
}
