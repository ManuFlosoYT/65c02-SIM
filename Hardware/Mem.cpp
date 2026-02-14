#include "Mem.h"

namespace Hardware {

void Mem::Init() {
    for (Byte& byte : memory) {
        byte = 0;
    }
}

void Mem::WriteWord(Word data, Word addr) {
    Write(addr, data & 0xFF);    // Write the least significant byte
    Write(addr + 1, data >> 8);  // Write the most significant byte
}

void Mem::Write(Word addr, Byte val) {
    auto it = writeHooks.find(addr);  // Find the hook
    if (it != writeHooks.end()) {
        it->second(addr, val);  // Execute the hook
    }
    memory[addr] = val;
}

void Mem::SetWriteHook(Word address, WriteHook hook) {
    writeHooks[address] = hook;  // Assign the hook
}

void Mem::SetReadHook(Word address, ReadHook hook) {
    readHooks[address] = hook;  // Assign the hook
}

Byte Mem::Read(Word addr) {
    auto it = readHooks.find(addr);  // Find the hook
    if (it != readHooks.end()) {
        return it->second(addr);  // Execute the hook
    }
    return memory[addr];
}

}  // namespace Hardware
