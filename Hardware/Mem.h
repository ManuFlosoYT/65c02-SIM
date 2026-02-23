#pragma once

#include <cstdint>
#include <iostream>

using Byte = uint8_t;
using Word = uint16_t;

using WriteHook = void (*)(void*, Word, Byte);
using ReadHook = Byte (*)(void*, Word);

#define ACIA_DATA 0x5000
#define ACIA_STATUS 0x5001
#define ACIA_CMD 0x5002
#define ACIA_CTRL 0x5003

// VIA Registers
#define PORTB 0x6000
#define PORTA 0x6001
#define DDRB 0x6002
#define DDRA 0x6003
#define T1C_L 0x6004
#define T1C_H 0x6005
#define T1L_L 0x6006
#define T1L_H 0x6007
#define T2C_L 0x6008
#define T2C_H 0x6009
#define SR 0x600A
#define ACR 0x600B
#define PCR 0x600C
#define IFR 0x600D
#define IER 0x600E
#define ORA_NH 0x600F

namespace Hardware {

class Mem {
public:
    constexpr static Word MAX_MEM = 0xFFFF;              // 64KB memory
    constexpr static Word ROM_SIZE = (MAX_MEM / 2) + 1;  // 32KB ROM

    Mem() { Init(); }
    Byte memory[MAX_MEM]{};
    void Init();

    Byte operator[](Word addr) const { return memory[addr]; }

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& in);

    void SetWriteHook(Word address, WriteHook hook, void* context = nullptr);
    inline void Write(Word addr, Byte val) {
        if (profilingEnabled) profilerCounts[addr]++;
        if (hasWriteHook[addr]) {
            writeHooks[addr](writeContext[addr], addr, val);  // Execute the hook
        }
        // Don't write to ROM
        if (addr < 0x8000) {
            memory[addr] = val;
        }
    }

    void WriteROM(Word addr, Byte val);  // Used by Unit Tests to force writes
    void WriteWord(Word data, Word addr);

    void SetReadHook(Word address, ReadHook hook, void* context = nullptr);

    inline Byte Read(Word addr) {
        if (profilingEnabled) profilerCounts[addr]++;
        if (hasReadHook[addr]) {
            return readHooks[addr](readContext[addr], addr);  // Execute the hook
        }
        return memory[addr];
    }

    inline Byte Peek(Word addr) const { return memory[addr]; }

    void ClearProfiler() {
        for (int i = 0; i <= MAX_MEM; i++) profilerCounts[i] = 0;
    }

    void SetProfilingEnabled(bool enabled) { profilingEnabled = enabled; }
    uint32_t* GetProfilerCounts() { return profilerCounts; }

private:
    WriteHook writeHooks[MAX_MEM + 1]{nullptr};
    void* writeContext[MAX_MEM + 1]{nullptr};
    bool hasWriteHook[MAX_MEM + 1]{false};

    ReadHook readHooks[MAX_MEM + 1]{nullptr};
    void* readContext[MAX_MEM + 1]{nullptr};
    bool hasReadHook[MAX_MEM + 1]{false};

    bool profilingEnabled = false;
    uint32_t profilerCounts[MAX_MEM + 1]{0};
};

}  // namespace Hardware
