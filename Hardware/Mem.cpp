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

void Mem::SetWriteHook(Word address, WriteHook hook, void* context) {
    writeHooks[address] = hook;
    writeContext[address] = context;
    hasWriteHook[address] = true;
}

void Mem::WriteROM(Word addr, Byte val) { memory[addr] = val; }

void Mem::SetReadHook(Word address, ReadHook hook, void* context) {
    readHooks[address] = hook;
    readContext[address] = context;
    hasReadHook[address] = true;
}

bool Mem::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(memory), MAX_MEM + 1);
    return out.good();
}

bool Mem::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(memory), MAX_MEM + 1);
    return in.good();
}

}  // namespace Hardware
