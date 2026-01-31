#ifndef MIPS_SIM_65C02_MEM_H
#define MIPS_SIM_65C02_MEM_H

#include <cstdint>

using Byte = uint8_t;
using Word = uint16_t;

class Mem {
public:
  constexpr static Word MAX_MEM = 0xFFFF; // 64KB de memoria
  Byte memoria[MAX_MEM]{};
  void Init();

  Byte &operator[](Word addr) { return memoria[addr]; }

  Byte operator[](Word addr) const { return memoria[addr]; }
};

#endif // MIPS_SIM_65C02_MEM_H