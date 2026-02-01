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
constexpr static Byte INS_LDA_IND_ZP = 0xB2;  // LDA IND ZP

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

constexpr static Byte INS_TXA = 0x8A; // TXA
constexpr static Byte INS_TAX = 0xAA; // TAX

constexpr static Byte INS_TYA = 0x98; // TYA
constexpr static Byte INS_TAY = 0xA8; // TAY

constexpr static Byte INS_AND_IM = 0x29; // AND Inmediato
constexpr static Byte INS_AND_ZP = 0x25; // AND ZP
constexpr static Byte INS_AND_ZPX = 0x35; // AND ZPX
constexpr static Byte INS_AND_ABS = 0x2D; // AND ABS
constexpr static Byte INS_AND_ABSX = 0x3D; // AND ABSX
constexpr static Byte INS_AND_ABSY = 0x39; // AND ABSY
constexpr static Byte INS_AND_INDX = 0x21; // AND INDX
constexpr static Byte INS_AND_INDY = 0x31; // AND INDY
constexpr static Byte INS_AND_IND_ZP = 0x32; // AND IND ZP

constexpr static Byte INS_ORA_IM = 0x09; // ORA Inmediato
constexpr static Byte INS_ORA_ZP = 0x05; // ORA ZP
constexpr static Byte INS_ORA_ZPX = 0x15; // ORA ZPX
constexpr static Byte INS_ORA_ABS = 0x0D; // ORA ABS
constexpr static Byte INS_ORA_ABSX = 0x1D; // ORA ABSX
constexpr static Byte INS_ORA_ABSY = 0x19; // ORA ABSY
constexpr static Byte INS_ORA_INDX = 0x01; // ORA INDX
constexpr static Byte INS_ORA_INDY = 0x11; // ORA INDY
constexpr static Byte INS_ORA_IND_ZP = 0x12; // ORA IND ZP

constexpr static Byte INS_EOR_IM = 0x49; // EOR Inmediato
constexpr static Byte INS_EOR_ZP = 0x45; // EOR ZP
constexpr static Byte INS_EOR_ZPX = 0x55; // EOR ZPX
constexpr static Byte INS_EOR_ABS = 0x4D; // EOR ABS
constexpr static Byte INS_EOR_ABSX = 0x5D; // EOR ABSX
constexpr static Byte INS_EOR_ABSY = 0x59; // EOR ABSY
constexpr static Byte INS_EOR_INDX = 0x41; // EOR INDX
constexpr static Byte INS_EOR_INDY = 0x51; // EOR INDY
constexpr static Byte INS_EOR_IND_ZP = 0x52; // EOR IND ZP

constexpr static Byte INS_BIT_IM = 0x89;    // BIT IM
constexpr static Byte INS_BIT_ZP = 0x24;    // BIT ZP
constexpr static Byte INS_BIT_ZPX = 0x34;   // BIT ZPX
constexpr static Byte INS_BIT_ABS = 0x2C;   // BIT ABS
constexpr static Byte INS_BIT_ABSX = 0x3C;  // BIT ABSX

constexpr static Byte INS_INC_A = 0x1A; // INC A
constexpr static Byte INS_INC_ZP = 0xE6; // INC ZP
constexpr static Byte INS_INC_ZPX = 0xF6; // INC ZPX
constexpr static Byte INS_INC_ABS = 0xEE; // INC ABS
constexpr static Byte INS_INC_ABSX = 0xFE; // INC ABSX

constexpr static Byte INS_DEC_A = 0x3A; // DEC A
constexpr static Byte INS_DEC_ZP = 0xC6; // DEC ZP
constexpr static Byte INS_DEC_ZPX = 0xD6; // DEC ZPX
constexpr static Byte INS_DEC_ABS = 0xCE; // DEC ABS
constexpr static Byte INS_DEC_ABSX = 0xDE; // DEC ABSX

constexpr static Byte INS_INX = 0xE8; // INX A
constexpr static Byte INS_DEX = 0xCA; // DEX A

constexpr static Byte INS_INY = 0xC8; // INY A
constexpr static Byte INS_DEY = 0x88; // DEY A

#endif  // MIPS_SIM_65C02_LISTAINSTRUCCIONES_H