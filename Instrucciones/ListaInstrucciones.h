#ifndef MIPS_SIM_65C02_LISTAINSTRUCCIONES_H
#define MIPS_SIM_65C02_LISTAINSTRUCCIONES_H

#include <cstdint>

using Byte = uint8_t;
using Word = uint16_t;

constexpr static Byte INS_NOP       = 0xEA; // NOP

constexpr static Byte INS_LDA_IM    = 0xA9; // LDA Inmediato
constexpr static Byte INS_LDA_ZP    = 0xA5; // LDA ZP
constexpr static Byte INS_LDA_ZPX   = 0xB5; // LDA ZPX
constexpr static Byte INS_LDA_ABS   = 0xAD; // LDA ABS
constexpr static Byte INS_LDA_ABSX  = 0xBD; // LDA ABSX
constexpr static Byte INS_LDA_ABSY  = 0xB9; // LDA ABSY
constexpr static Byte INS_LDA_INDX  = 0xA1; // LDA INDX
constexpr static Byte INS_LDA_INDY  = 0xB1; // LDA INDY

#endif //MIPS_SIM_65C02_LISTAINSTRUCCIONES_H