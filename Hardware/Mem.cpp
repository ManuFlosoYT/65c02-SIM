#include "Mem.h"

namespace Hardware {

void Mem::Init() {
    for (Byte& byte : memory) {
        byte = 0;
    }
    for (uint32_t i = 0; i <= MAX_MEM; i++) {
        hasWriteHook[i] = false;
        hasReadHook[i] = false;
    }
}

void Mem::WriteWord(Word data, Word addr) {
    Write(addr, data & 0xFF);    // Write the least significant byte
    Write(addr + 1, data >> 8);  // Write the most significant byte
}

void Mem::Write(Word addr, Byte val) {
    if (hasWriteHook[addr]) {
        writeHooks[addr](addr, val);  // Execute the hook
    }
    // Don't write to ROM
    if (addr < 0x8000) {
        memory[addr] = val;
    }
}

void Mem::SetWriteHook(Word address, WriteHook hook) {
    writeHooks[address] = hook;  // Assign the hook
    hasWriteHook[address] = true;
}

void Mem::WriteROM(Word addr, Byte val) { memory[addr] = val; }

void Mem::SetReadHook(Word address, ReadHook hook) {
    readHooks[address] = hook;  // Assign the hook
    hasReadHook[address] = true;
}

Byte Mem::Read(Word addr) {
    if (hasReadHook[addr]) {
        return readHooks[addr](addr);  // Execute the hook
    }
    return memory[addr];
}

}  // namespace Hardware
