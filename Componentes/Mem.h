#ifndef SIM_65C02_MEM_H
#define SIM_65C02_MEM_H

#include <cstdint>
#include <functional>

using Byte = uint8_t;
using Word = uint16_t;

#define PORTB 0x6000
#define DDRB 0x6002

class Mem {
public:
    constexpr static Word MAX_MEM = 0xFFFF;              // 64KB de memoria
    constexpr static Word ROM_SIZE = (MAX_MEM / 2) + 1;  // 32KB de ROM

    Mem() { Init(); }
    Byte memoria[MAX_MEM]{};
    void Init();

    Byte operator[](Word dir) const { return memoria[dir]; }

    using WriteHook = std::function<void(Word, Byte)>;
    void SetWriteHook(WriteHook hook);

    void Write(Word dir, Byte val);
    void WriteWord(Word dato, Word dir);

private:
    WriteHook writeHook;
};

#endif  // SIM_65C02_MEM_H