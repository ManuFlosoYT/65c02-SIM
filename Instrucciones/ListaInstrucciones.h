//
// Created by manu on 29/1/26.
//

#ifndef MIPS_SIM_65C02_LISTAINSTRUCCIONES_H
#define MIPS_SIM_65C02_LISTAINSTRUCCIONES_H
#include <cstdint>

using Byte = uint8_t;
using Word = uint16_t;

constexpr static Byte INS_NOP       = 0xEA; // NOP
constexpr static Byte INS_LDA_IM    = 0xA9; // LDA Inmediato

#endif //MIPS_SIM_65C02_LISTAINSTRUCCIONES_H