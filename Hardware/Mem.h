#pragma once

#include <cstdint>
#include <functional>
#include <map>

using Byte = uint8_t;
using Word = uint16_t;

using WriteHook = std::function<void(Word, Byte)>;
using ReadHook = std::function<Byte(Word)>;

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

    void SetWriteHook(Word address, WriteHook hook);
    void Write(Word addr, Byte val);    // Protects against writing to ROM
    void WriteROM(Word addr, Byte val); // Used by Unit Tests to force writes
    void WriteWord(Word data, Word addr);

    void SetReadHook(Word address, ReadHook hook);
    Byte Read(Word addr);

private:
    std::map<Word, WriteHook> writeHooks;
    std::map<Word, ReadHook> readHooks;
};

}  // namespace Hardware
