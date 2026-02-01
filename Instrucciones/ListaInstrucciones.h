#ifndef MIPS_SIM_65C02_LISTAINSTRUCCIONES_H
#define MIPS_SIM_65C02_LISTAINSTRUCCIONES_H

#include <cstdint>

using Byte = uint8_t;
using Word = uint16_t;

constexpr static Byte INS_NOP = 0xEA;  // NOP

constexpr static Byte INS_LDA_IM = 0xA9;    // LDA Inmediato
constexpr static Byte INS_LDA_ZP = 0xA5;    // LDA ZP
constexpr static Byte INS_LDA_ZPX = 0xB5;   // LDA ZPX
constexpr static Byte INS_LDA_ABS = 0xAD;   // LDA ABS
constexpr static Byte INS_LDA_ABSX = 0xBD;  // LDA ABSX
constexpr static Byte INS_LDA_ABSY = 0xB9;  // LDA ABSY
constexpr static Byte INS_LDA_INDX = 0xA1;  // LDA INDX
constexpr static Byte INS_LDA_INDY = 0xB1;  // LDA INDY

constexpr static Byte INS_LDX_IM = 0xA2;    // LDX Inmediato
constexpr static Byte INS_LDX_ZP = 0xA6;    // LDX ZP
constexpr static Byte INS_LDX_ZPY = 0xB6;   // LDX ZPY
constexpr static Byte INS_LDX_ABS = 0xAE;   // LDX ABS
constexpr static Byte INS_LDX_ABSY = 0xBE;  // LDX ABSY

constexpr static Byte INS_LDY_IM = 0xA0;    // LDY Inmediato
constexpr static Byte INS_LDY_ZP = 0xA4;    // LDY ZP
constexpr static Byte INS_LDY_ZPX = 0xB4;   // LDY ZPX
constexpr static Byte INS_LDY_ABS = 0xAC;   // LDY ABS
constexpr static Byte INS_LDY_ABSX = 0xBC;  // LDY ABSX

constexpr static Byte INS_STA_ZP = 0x85;      // STA ZP
constexpr static Byte INS_STA_ZPX = 0x95;     // STA ZPX
constexpr static Byte INS_STA_ABS = 0x8D;     // STA ABS
constexpr static Byte INS_STA_ABSX = 0x9D;    // STA ABSX
constexpr static Byte INS_STA_ABSY = 0x99;    // STA ABSY
constexpr static Byte INS_STA_INDX = 0x81;    // STA INDX
constexpr static Byte INS_STA_INDY = 0x91;    // STA INDY
constexpr static Byte INS_STA_IND_ZP = 0x92;  // STA IND ZP

constexpr static Byte INS_STX_ZP = 0x86;   // STX ZP
constexpr static Byte INS_STX_ZPY = 0x96;  // STX ZPY
constexpr static Byte INS_STX_ABS = 0x8E;  // STX ABS

constexpr static Byte INS_STY_ZP = 0x84;   // STY ZP
constexpr static Byte INS_STY_ZPX = 0x94;  // STY ZPX
constexpr static Byte INS_STY_ABS = 0x8C;  // STY ABS

constexpr static Byte INS_JSR = 0x20;  // JSR
constexpr static Byte INS_RTS = 0x60;  // RTS

constexpr static Byte INS_JMP_ABS = 0x4C;   // JMP ABS
constexpr static Byte INS_JMP_IND = 0x6C;   // JMP IND
constexpr static Byte INS_JMP_ABSX = 0x7C;  // JMP ABSX

constexpr static Byte INS_TSX = 0xBA; // TSX
constexpr static Byte INS_TXS = 0x9A; // TXS

constexpr static Byte INS_PHA = 0x48; // PHA
constexpr static Byte INS_PLA = 0x68; // PLA

constexpr static Byte INS_PHP = 0x08; // PHP
constexpr static Byte INS_PLP = 0x28; // PLP

constexpr static Byte INS_PHX = 0xDA; // PHX
constexpr static Byte INS_PLX = 0xFA; // PLX

constexpr static Byte INS_PHY = 0x5A; // PHY
constexpr static Byte INS_PLY = 0x7A; // PLY

#endif  // MIPS_SIM_65C02_LISTAINSTRUCCIONES_H