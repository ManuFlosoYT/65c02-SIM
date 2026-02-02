#ifndef SIM_65C02_MEM_H
#define SIM_65C02_MEM_H

#include <cstdint>

using Byte = uint8_t;
using Word = uint16_t;

class Mem {
public:
    constexpr static Word MAX_MEM = 0xFFFF;  // 64KB de memoria
    Byte memoria[MAX_MEM]{};
    void Init();

    Byte& operator[](Word dir) { return memoria[dir]; }
    Byte operator[](Word dir) const { return memoria[dir]; }

    void WriteWord(Word dato, Word dir);
};

#endif  // SIM_65C02_MEM_H