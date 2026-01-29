//
// Created by manu on 29/1/26.
//

#ifndef MIPS_SIM_65C02_MEM_H
#define MIPS_SIM_65C02_MEM_H
#include <cstdint>

using Byte = uint8_t;
using Word = uint16_t;

class Mem {
    private:
        constexpr static uint32_t MAX_MEM = 65536; // 64KB de memoria
        Byte memoria[MAX_MEM]{};
    public:
        void Init();
};


#endif //MIPS_SIM_65C02_MEM_H