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

constexpr static Byte INS_LDX_IM    = 0xA2; // LDX Inmediato
constexpr static Byte INS_LDX_ZP    = 0xA6; // LDX ZP
constexpr static Byte INS_LDX_ZPY   = 0xB6; // LDX ZPY
constexpr static Byte INS_LDX_ABS   = 0xAE; // LDX ABS
constexpr static Byte INS_LDX_ABSY  = 0xBE; // LDX ABSY

constexpr static Byte INS_LDY_IM    = 0xA0; // LDY Inmediato
constexpr static Byte INS_LDY_ZP    = 0xA4; // LDY ZP
constexpr static Byte INS_LDY_ZPX   = 0xB4; // LDY ZPX
constexpr static Byte INS_LDY_ABS   = 0xAC; // LDY ABS
constexpr static Byte INS_LDY_ABSX  = 0xBC; // LDY ABSX

constexpr static Byte INS_JSR       = 0x20; // JSR

constexpr static Byte INS_JMP_ABS   = 0x4C; // JMP ABS
constexpr static Byte INS_JMP_IND   = 0x6C; // JMP IND
constexpr static Byte INS_JMP_ABSX = 0x7C; // JMP ABSX

#endif //MIPS_SIM_65C02_LISTAINSTRUCCIONES_H