#pragma once

#include <array>
#include <cstdint>

namespace Hardware {
class CPU;
class Bus;

using Byte = uint8_t;
using Word = uint16_t;

constexpr static Byte INS_WAI = 0xCB;  // WAI
constexpr static Byte INS_STP = 0xDB;  // STP

constexpr static Byte INS_NOP = 0xEA;  // NOP
constexpr static Byte INS_BRK = 0x00;  // BRK
constexpr static Byte INS_RTI = 0x40;  // RTI
constexpr static Byte INS_JAM = 0x02;  // JAM

constexpr static Byte INS_LDA_IM = 0xA9;      // LDA Immediate
constexpr static Byte INS_LDA_ZP = 0xA5;      // LDA ZP
constexpr static Byte INS_LDA_ZPX = 0xB5;     // LDA ZPX
constexpr static Byte INS_LDA_ABS = 0xAD;     // LDA ABS
constexpr static Byte INS_LDA_ABSX = 0xBD;    // LDA ABSX
constexpr static Byte INS_LDA_ABSY = 0xB9;    // LDA ABSY
constexpr static Byte INS_LDA_INDX = 0xA1;    // LDA INDX
constexpr static Byte INS_LDA_INDY = 0xB1;    // LDA INDY
constexpr static Byte INS_LDA_IND_ZP = 0xB2;  // LDA IND ZP

constexpr static Byte INS_LDX_IM = 0xA2;    // LDX Immediate
constexpr static Byte INS_LDX_ZP = 0xA6;    // LDX ZP
constexpr static Byte INS_LDX_ZPY = 0xB6;   // LDX ZPY
constexpr static Byte INS_LDX_ABS = 0xAE;   // LDX ABS
constexpr static Byte INS_LDX_ABSY = 0xBE;  // LDX ABSY

constexpr static Byte INS_LDY_IM = 0xA0;    // LDY Immediate
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

constexpr static Byte INS_TSX = 0xBA;  // TSX
constexpr static Byte INS_TXS = 0x9A;  // TXS

constexpr static Byte INS_PHA = 0x48;  // PHA
constexpr static Byte INS_PLA = 0x68;  // PLA

constexpr static Byte INS_PHP = 0x08;  // PHP
constexpr static Byte INS_PLP = 0x28;  // PLP

constexpr static Byte INS_PHX = 0xDA;  // PHX
constexpr static Byte INS_PLX = 0xFA;  // PLX

constexpr static Byte INS_PHY = 0x5A;  // PHY
constexpr static Byte INS_PLY = 0x7A;  // PLY

constexpr static Byte INS_TXA = 0x8A;  // TXA
constexpr static Byte INS_TAX = 0xAA;  // TAX

constexpr static Byte INS_TYA = 0x98;  // TYA
constexpr static Byte INS_TAY = 0xA8;  // TAY

constexpr static Byte INS_AND_IM = 0x29;      // AND Immediate
constexpr static Byte INS_AND_ZP = 0x25;      // AND ZP
constexpr static Byte INS_AND_ZPX = 0x35;     // AND ZPX
constexpr static Byte INS_AND_ABS = 0x2D;     // AND ABS
constexpr static Byte INS_AND_ABSX = 0x3D;    // AND ABSX
constexpr static Byte INS_AND_ABSY = 0x39;    // AND ABSY
constexpr static Byte INS_AND_INDX = 0x21;    // AND INDX
constexpr static Byte INS_AND_INDY = 0x31;    // AND INDY
constexpr static Byte INS_AND_IND_ZP = 0x32;  // AND IND ZP

constexpr static Byte INS_ORA_IM = 0x09;      // ORA Immediate
constexpr static Byte INS_ORA_ZP = 0x05;      // ORA ZP
constexpr static Byte INS_ORA_ZPX = 0x15;     // ORA ZPX
constexpr static Byte INS_ORA_ABS = 0x0D;     // ORA ABS
constexpr static Byte INS_ORA_ABSX = 0x1D;    // ORA ABSX
constexpr static Byte INS_ORA_ABSY = 0x19;    // ORA ABSY
constexpr static Byte INS_ORA_INDX = 0x01;    // ORA INDX
constexpr static Byte INS_ORA_INDY = 0x11;    // ORA INDY
constexpr static Byte INS_ORA_IND_ZP = 0x12;  // ORA IND ZP

constexpr static Byte INS_EOR_IM = 0x49;      // EOR Immediate
constexpr static Byte INS_EOR_ZP = 0x45;      // EOR ZP
constexpr static Byte INS_EOR_ZPX = 0x55;     // EOR ZPX
constexpr static Byte INS_EOR_ABS = 0x4D;     // EOR ABS
constexpr static Byte INS_EOR_ABSX = 0x5D;    // EOR ABSX
constexpr static Byte INS_EOR_ABSY = 0x59;    // EOR ABSY
constexpr static Byte INS_EOR_INDX = 0x41;    // EOR INDX
constexpr static Byte INS_EOR_INDY = 0x51;    // EOR INDY
constexpr static Byte INS_EOR_IND_ZP = 0x52;  // EOR IND ZP

constexpr static Byte INS_BIT_IM = 0x89;    // BIT IM
constexpr static Byte INS_BIT_ZP = 0x24;    // BIT ZP
constexpr static Byte INS_BIT_ZPX = 0x34;   // BIT ZPX
constexpr static Byte INS_BIT_ABS = 0x2C;   // BIT ABS
constexpr static Byte INS_BIT_ABSX = 0x3C;  // BIT ABSX

constexpr static Byte INS_INC_A = 0x1A;     // INC A
constexpr static Byte INS_INC_ZP = 0xE6;    // INC ZP
constexpr static Byte INS_INC_ZPX = 0xF6;   // INC ZPX
constexpr static Byte INS_INC_ABS = 0xEE;   // INC ABS
constexpr static Byte INS_INC_ABSX = 0xFE;  // INC ABSX

constexpr static Byte INS_DEC_A = 0x3A;     // DEC A
constexpr static Byte INS_DEC_ZP = 0xC6;    // DEC ZP
constexpr static Byte INS_DEC_ZPX = 0xD6;   // DEC ZPX
constexpr static Byte INS_DEC_ABS = 0xCE;   // DEC ABS
constexpr static Byte INS_DEC_ABSX = 0xDE;  // DEC ABSX

constexpr static Byte INS_INX = 0xE8;  // INX A
constexpr static Byte INS_DEX = 0xCA;  // DEX A

constexpr static Byte INS_INY = 0xC8;  // INY A
constexpr static Byte INS_DEY = 0x88;  // DEY A

constexpr static Byte INS_ADC_IM = 0x69;      // ADC Immediate
constexpr static Byte INS_ADC_ZP = 0x65;      // ADC ZP
constexpr static Byte INS_ADC_ZPX = 0x75;     // ADC ZPX
constexpr static Byte INS_ADC_ABS = 0x6D;     // ADC ABS
constexpr static Byte INS_ADC_ABSX = 0x7D;    // ADC ABSX
constexpr static Byte INS_ADC_ABSY = 0x79;    // ADC ABSY
constexpr static Byte INS_ADC_INDX = 0x61;    // ADC INDX
constexpr static Byte INS_ADC_INDY = 0x71;    // ADC INDY
constexpr static Byte INS_ADC_IND_ZP = 0x72;  // ADC IND ZP

constexpr static Byte INS_SBC_IM = 0xE9;      // SBC Immediate
constexpr static Byte INS_SBC_ZP = 0xE5;      // SBC ZP
constexpr static Byte INS_SBC_ZPX = 0xF5;     // SBC ZPX
constexpr static Byte INS_SBC_ABS = 0xED;     // SBC ABS
constexpr static Byte INS_SBC_ABSX = 0xFD;    // SBC ABSX
constexpr static Byte INS_SBC_ABSY = 0xF9;    // SBC ABSY
constexpr static Byte INS_SBC_INDX = 0xE1;    // SBC INDX
constexpr static Byte INS_SBC_INDY = 0xF1;    // SBC INDY
constexpr static Byte INS_SBC_IND_ZP = 0xF2;  // SBC IND ZP

constexpr static Byte INS_CMP_IM = 0xC9;      // CMP Immediate
constexpr static Byte INS_CMP_ZP = 0xC5;      // CMP ZP
constexpr static Byte INS_CMP_ZPX = 0xD5;     // CMP ZPX
constexpr static Byte INS_CMP_ABS = 0xCD;     // CMP ABS
constexpr static Byte INS_CMP_ABSX = 0xDD;    // CMP ABSX
constexpr static Byte INS_CMP_ABSY = 0xD9;    // CMP ABSY
constexpr static Byte INS_CMP_INDX = 0xC1;    // CMP INDX
constexpr static Byte INS_CMP_INDY = 0xD1;    // CMP INDY
constexpr static Byte INS_CMP_IND_ZP = 0xD2;  // CMP IND ZP

constexpr static Byte INS_CPX_IM = 0xE0;   // CPX Immediate
constexpr static Byte INS_CPX_ZP = 0xE4;   // CPX ZP
constexpr static Byte INS_CPX_ABS = 0xEC;  // CPX ABS

constexpr static Byte INS_CPY_IM = 0xC0;   // CPY Immediate
constexpr static Byte INS_CPY_ZP = 0xC4;   // CPY ZP
constexpr static Byte INS_CPY_ABS = 0xCC;  // CPY ABS

constexpr static Byte INS_ASL_A = 0x0A;     // ASL A
constexpr static Byte INS_ASL_ZP = 0x06;    // ASL ZP
constexpr static Byte INS_ASL_ZPX = 0x16;   // ASL ZPX
constexpr static Byte INS_ASL_ABS = 0x0E;   // ASL ABS
constexpr static Byte INS_ASL_ABSX = 0x1E;  // ASL ABSX

constexpr static Byte INS_LSR_A = 0x4A;     // LSR A
constexpr static Byte INS_LSR_ZP = 0x46;    // LSR ZP
constexpr static Byte INS_LSR_ZPX = 0x56;   // LSR ZPX
constexpr static Byte INS_LSR_ABS = 0x4E;   // LSR ABS
constexpr static Byte INS_LSR_ABSX = 0x5E;  // LSR ABSX

constexpr static Byte INS_ROL_A = 0x2A;     // ROL A
constexpr static Byte INS_ROL_ZP = 0x26;    // ROL ZP
constexpr static Byte INS_ROL_ZPX = 0x36;   // ROL ZPX
constexpr static Byte INS_ROL_ABS = 0x2E;   // ROL ABS
constexpr static Byte INS_ROL_ABSX = 0x3E;  // ROL ABSX

constexpr static Byte INS_ROR_A = 0x6A;     // ROR A
constexpr static Byte INS_ROR_ZP = 0x66;    // ROR ZP
constexpr static Byte INS_ROR_ZPX = 0x76;   // ROR ZPX
constexpr static Byte INS_ROR_ABS = 0x6E;   // ROR ABS
constexpr static Byte INS_ROR_ABSX = 0x7E;  // ROR ABSX

constexpr static Byte INS_BCC = 0x90;  // BCC
constexpr static Byte INS_BCS = 0xB0;  // BCS
constexpr static Byte INS_BEQ = 0xF0;  // BEQ
constexpr static Byte INS_BMI = 0x30;  // BMI
constexpr static Byte INS_BNE = 0xD0;  // BNE
constexpr static Byte INS_BPL = 0x10;  // BPL
constexpr static Byte INS_BVC = 0x50;  // BVC
constexpr static Byte INS_BVS = 0x70;  // BVS

constexpr static Byte INS_CLC = 0x18;  // CLC
constexpr static Byte INS_SEC = 0x38;  // SEC
constexpr static Byte INS_CLI = 0x58;  // CLI
constexpr static Byte INS_SEI = 0x78;  // SEI
constexpr static Byte INS_CLD = 0xD8;  // CLD
constexpr static Byte INS_SED = 0xF8;  // SED
constexpr static Byte INS_CLV = 0xB8;  // CLV

constexpr static Byte INS_STZ_ZP = 0x64;    // STZ ZP
constexpr static Byte INS_STZ_ZPX = 0x74;   // STZ ZPX
constexpr static Byte INS_STZ_ABS = 0x9C;   // STZ ABS
constexpr static Byte INS_STZ_ABSX = 0x9E;  // STZ ABSX

constexpr static Byte INS_BRA = 0x80;  // BRA (Branch Always)

constexpr static Byte INS_RMB0 = 0x07;  // RMB0
constexpr static Byte INS_RMB1 = 0x17;  // RMB1
constexpr static Byte INS_RMB2 = 0x27;  // RMB2
constexpr static Byte INS_RMB3 = 0x37;  // RMB3
constexpr static Byte INS_RMB4 = 0x47;  // RMB4
constexpr static Byte INS_RMB5 = 0x57;  // RMB5
constexpr static Byte INS_RMB6 = 0x67;  // RMB6
constexpr static Byte INS_RMB7 = 0x77;  // RMB7

constexpr static Byte INS_SMB0 = 0x87;  // SMB0
constexpr static Byte INS_SMB1 = 0x97;  // SMB1
constexpr static Byte INS_SMB2 = 0xA7;  // SMB2
constexpr static Byte INS_SMB3 = 0xB7;  // SMB3
constexpr static Byte INS_SMB4 = 0xC7;  // SMB4
constexpr static Byte INS_SMB5 = 0xD7;  // SMB5
constexpr static Byte INS_SMB6 = 0xE7;  // SMB6
constexpr static Byte INS_SMB7 = 0xF7;  // SMB7

constexpr static Byte INS_BBR0 = 0x0F;  // BBR0
constexpr static Byte INS_BBR1 = 0x1F;  // BBR1
constexpr static Byte INS_BBR2 = 0x2F;  // BBR2
constexpr static Byte INS_BBR3 = 0x3F;  // BBR3
constexpr static Byte INS_BBR4 = 0x4F;  // BBR4
constexpr static Byte INS_BBR5 = 0x5F;  // BBR5
constexpr static Byte INS_BBR6 = 0x6F;  // BBR6
constexpr static Byte INS_BBR7 = 0x7F;  // BBR7

constexpr static Byte INS_BBS0 = 0x8F;  // BBS0
constexpr static Byte INS_BBS1 = 0x9F;  // BBS1
constexpr static Byte INS_BBS2 = 0xAF;  // BBS2
constexpr static Byte INS_BBS3 = 0xBF;  // BBS3
constexpr static Byte INS_BBS4 = 0xCF;  // BBS4
constexpr static Byte INS_BBS5 = 0xDF;  // BBS5
constexpr static Byte INS_BBS6 = 0xEF;  // BBS6
constexpr static Byte INS_BBS7 = 0xFF;  // BBS7

constexpr static Byte INS_TRB_ABS = 0x1C;  // TRB ABS
constexpr static Byte INS_TRB_ZP = 0x14;   // TRB ZP

constexpr static Byte INS_TSB_ABS = 0x0C;  // TSB ABS
constexpr static Byte INS_TSB_ZP = 0x04;   // TSB ZP

// Cycle counts for each instruction
constexpr static Byte CYC_INS_WAI = 3;
constexpr static Byte CYC_INS_STP = 3;
constexpr static Byte CYC_INS_NOP = 2;
constexpr static Byte CYC_INS_BRK = 7;
constexpr static Byte CYC_INS_RTI = 6;
constexpr static Byte CYC_INS_JAM = 2;

constexpr static Byte CYC_INS_LDA_IM = 2;
constexpr static Byte CYC_INS_LDA_ZP = 3;
constexpr static Byte CYC_INS_LDA_ZPX = 4;
constexpr static Byte CYC_INS_LDA_ABS = 4;
constexpr static Byte CYC_INS_LDA_ABSX = 4;
constexpr static Byte CYC_INS_LDA_ABSY = 4;
constexpr static Byte CYC_INS_LDA_INDX = 6;
constexpr static Byte CYC_INS_LDA_INDY = 5;
constexpr static Byte CYC_INS_LDA_IND_ZP = 5;

constexpr static Byte CYC_INS_LDX_IM = 2;
constexpr static Byte CYC_INS_LDX_ZP = 3;
constexpr static Byte CYC_INS_LDX_ZPY = 4;
constexpr static Byte CYC_INS_LDX_ABS = 4;
constexpr static Byte CYC_INS_LDX_ABSY = 4;

constexpr static Byte CYC_INS_LDY_IM = 2;
constexpr static Byte CYC_INS_LDY_ZP = 3;
constexpr static Byte CYC_INS_LDY_ZPX = 4;
constexpr static Byte CYC_INS_LDY_ABS = 4;
constexpr static Byte CYC_INS_LDY_ABSX = 4;

constexpr static Byte CYC_INS_STA_ZP = 3;
constexpr static Byte CYC_INS_STA_ZPX = 4;
constexpr static Byte CYC_INS_STA_ABS = 4;
constexpr static Byte CYC_INS_STA_ABSX = 5;
constexpr static Byte CYC_INS_STA_ABSY = 5;
constexpr static Byte CYC_INS_STA_INDX = 6;
constexpr static Byte CYC_INS_STA_INDY = 6;
constexpr static Byte CYC_INS_STA_IND_ZP = 5;

constexpr static Byte CYC_INS_STX_ZP = 3;
constexpr static Byte CYC_INS_STX_ZPY = 4;
constexpr static Byte CYC_INS_STX_ABS = 4;

constexpr static Byte CYC_INS_STY_ZP = 3;
constexpr static Byte CYC_INS_STY_ZPX = 4;
constexpr static Byte CYC_INS_STY_ABS = 4;

constexpr static Byte CYC_INS_JSR = 6;
constexpr static Byte CYC_INS_RTS = 6;

constexpr static Byte CYC_INS_JMP_ABS = 3;
constexpr static Byte CYC_INS_JMP_IND = 5;
constexpr static Byte CYC_INS_JMP_ABSX = 6;

constexpr static Byte CYC_INS_TSX = 2;
constexpr static Byte CYC_INS_TXS = 2;

constexpr static Byte CYC_INS_PHA = 3;
constexpr static Byte CYC_INS_PLA = 4;
constexpr static Byte CYC_INS_PHP = 3;
constexpr static Byte CYC_INS_PLP = 4;
constexpr static Byte CYC_INS_PHX = 3;
constexpr static Byte CYC_INS_PLX = 4;
constexpr static Byte CYC_INS_PHY = 3;
constexpr static Byte CYC_INS_PLY = 4;

constexpr static Byte CYC_INS_TXA = 2;
constexpr static Byte CYC_INS_TAX = 2;
constexpr static Byte CYC_INS_TYA = 2;
constexpr static Byte CYC_INS_TAY = 2;

constexpr static Byte CYC_INS_AND_IM = 2;
constexpr static Byte CYC_INS_AND_ZP = 3;
constexpr static Byte CYC_INS_AND_ZPX = 4;
constexpr static Byte CYC_INS_AND_ABS = 4;
constexpr static Byte CYC_INS_AND_ABSX = 4;
constexpr static Byte CYC_INS_AND_ABSY = 4;
constexpr static Byte CYC_INS_AND_INDX = 6;
constexpr static Byte CYC_INS_AND_INDY = 5;
constexpr static Byte CYC_INS_AND_IND_ZP = 5;

constexpr static Byte CYC_INS_ORA_IM = 2;
constexpr static Byte CYC_INS_ORA_ZP = 3;
constexpr static Byte CYC_INS_ORA_ZPX = 4;
constexpr static Byte CYC_INS_ORA_ABS = 4;
constexpr static Byte CYC_INS_ORA_ABSX = 4;
constexpr static Byte CYC_INS_ORA_ABSY = 4;
constexpr static Byte CYC_INS_ORA_INDX = 6;
constexpr static Byte CYC_INS_ORA_INDY = 5;
constexpr static Byte CYC_INS_ORA_IND_ZP = 5;

constexpr static Byte CYC_INS_EOR_IM = 2;
constexpr static Byte CYC_INS_EOR_ZP = 3;
constexpr static Byte CYC_INS_EOR_ZPX = 4;
constexpr static Byte CYC_INS_EOR_ABS = 4;
constexpr static Byte CYC_INS_EOR_ABSX = 4;
constexpr static Byte CYC_INS_EOR_ABSY = 4;
constexpr static Byte CYC_INS_EOR_INDX = 6;
constexpr static Byte CYC_INS_EOR_INDY = 5;
constexpr static Byte CYC_INS_EOR_IND_ZP = 5;

constexpr static Byte CYC_INS_BIT_IM = 2;
constexpr static Byte CYC_INS_BIT_ZP = 3;
constexpr static Byte CYC_INS_BIT_ZPX = 4;
constexpr static Byte CYC_INS_BIT_ABS = 4;
constexpr static Byte CYC_INS_BIT_ABSX = 4;

constexpr static Byte CYC_INS_INC_A = 2;
constexpr static Byte CYC_INS_INC_ZP = 5;
constexpr static Byte CYC_INS_INC_ZPX = 6;
constexpr static Byte CYC_INS_INC_ABS = 6;
constexpr static Byte CYC_INS_INC_ABSX = 7;

constexpr static Byte CYC_INS_DEC_A = 2;
constexpr static Byte CYC_INS_DEC_ZP = 5;
constexpr static Byte CYC_INS_DEC_ZPX = 6;
constexpr static Byte CYC_INS_DEC_ABS = 6;
constexpr static Byte CYC_INS_DEC_ABSX = 7;

constexpr static Byte CYC_INS_INX = 2;
constexpr static Byte CYC_INS_DEX = 2;
constexpr static Byte CYC_INS_INY = 2;
constexpr static Byte CYC_INS_DEY = 2;

constexpr static Byte CYC_INS_ADC_IM = 2;
constexpr static Byte CYC_INS_ADC_ZP = 3;
constexpr static Byte CYC_INS_ADC_ZPX = 4;
constexpr static Byte CYC_INS_ADC_ABS = 4;
constexpr static Byte CYC_INS_ADC_ABSX = 4;
constexpr static Byte CYC_INS_ADC_ABSY = 4;
constexpr static Byte CYC_INS_ADC_INDX = 6;
constexpr static Byte CYC_INS_ADC_INDY = 5;
constexpr static Byte CYC_INS_ADC_IND_ZP = 5;

constexpr static Byte CYC_INS_SBC_IM = 2;
constexpr static Byte CYC_INS_SBC_ZP = 3;
constexpr static Byte CYC_INS_SBC_ZPX = 4;
constexpr static Byte CYC_INS_SBC_ABS = 4;
constexpr static Byte CYC_INS_SBC_ABSX = 4;
constexpr static Byte CYC_INS_SBC_ABSY = 4;
constexpr static Byte CYC_INS_SBC_INDX = 6;
constexpr static Byte CYC_INS_SBC_INDY = 5;
constexpr static Byte CYC_INS_SBC_IND_ZP = 5;

constexpr static Byte CYC_INS_CMP_IM = 2;
constexpr static Byte CYC_INS_CMP_ZP = 3;
constexpr static Byte CYC_INS_CMP_ZPX = 4;
constexpr static Byte CYC_INS_CMP_ABS = 4;
constexpr static Byte CYC_INS_CMP_ABSX = 4;
constexpr static Byte CYC_INS_CMP_ABSY = 4;
constexpr static Byte CYC_INS_CMP_INDX = 6;
constexpr static Byte CYC_INS_CMP_INDY = 5;
constexpr static Byte CYC_INS_CMP_IND_ZP = 5;

constexpr static Byte CYC_INS_CPX_IM = 2;
constexpr static Byte CYC_INS_CPX_ZP = 3;
constexpr static Byte CYC_INS_CPX_ABS = 4;

constexpr static Byte CYC_INS_CPY_IM = 2;
constexpr static Byte CYC_INS_CPY_ZP = 3;
constexpr static Byte CYC_INS_CPY_ABS = 4;

constexpr static Byte CYC_INS_ASL_A = 2;
constexpr static Byte CYC_INS_ASL_ZP = 5;
constexpr static Byte CYC_INS_ASL_ZPX = 6;
constexpr static Byte CYC_INS_ASL_ABS = 6;
constexpr static Byte CYC_INS_ASL_ABSX = 7;

constexpr static Byte CYC_INS_LSR_A = 2;
constexpr static Byte CYC_INS_LSR_ZP = 5;
constexpr static Byte CYC_INS_LSR_ZPX = 6;
constexpr static Byte CYC_INS_LSR_ABS = 6;
constexpr static Byte CYC_INS_LSR_ABSX = 7;

constexpr static Byte CYC_INS_ROL_A = 2;
constexpr static Byte CYC_INS_ROL_ZP = 5;
constexpr static Byte CYC_INS_ROL_ZPX = 6;
constexpr static Byte CYC_INS_ROL_ABS = 6;
constexpr static Byte CYC_INS_ROL_ABSX = 7;

constexpr static Byte CYC_INS_ROR_A = 2;
constexpr static Byte CYC_INS_ROR_ZP = 5;
constexpr static Byte CYC_INS_ROR_ZPX = 6;
constexpr static Byte CYC_INS_ROR_ABS = 6;
constexpr static Byte CYC_INS_ROR_ABSX = 7;

constexpr static Byte CYC_INS_BCC = 2;
constexpr static Byte CYC_INS_BCS = 2;
constexpr static Byte CYC_INS_BEQ = 2;
constexpr static Byte CYC_INS_BMI = 2;
constexpr static Byte CYC_INS_BNE = 2;
constexpr static Byte CYC_INS_BPL = 2;
constexpr static Byte CYC_INS_BVC = 2;
constexpr static Byte CYC_INS_BVS = 2;

constexpr static Byte CYC_INS_CLC = 2;
constexpr static Byte CYC_INS_SEC = 2;
constexpr static Byte CYC_INS_CLI = 2;
constexpr static Byte CYC_INS_SEI = 2;
constexpr static Byte CYC_INS_CLD = 2;
constexpr static Byte CYC_INS_SED = 2;
constexpr static Byte CYC_INS_CLV = 2;

constexpr static Byte CYC_INS_STZ_ZP = 3;
constexpr static Byte CYC_INS_STZ_ZPX = 4;
constexpr static Byte CYC_INS_STZ_ABS = 4;
constexpr static Byte CYC_INS_STZ_ABSX = 5;

constexpr static Byte CYC_INS_BRA = 3;

constexpr static Byte CYC_INS_RMB0 = 5;
constexpr static Byte CYC_INS_RMB1 = 5;
constexpr static Byte CYC_INS_RMB2 = 5;
constexpr static Byte CYC_INS_RMB3 = 5;
constexpr static Byte CYC_INS_RMB4 = 5;
constexpr static Byte CYC_INS_RMB5 = 5;
constexpr static Byte CYC_INS_RMB6 = 5;
constexpr static Byte CYC_INS_RMB7 = 5;

constexpr static Byte CYC_INS_SMB0 = 5;
constexpr static Byte CYC_INS_SMB1 = 5;
constexpr static Byte CYC_INS_SMB2 = 5;
constexpr static Byte CYC_INS_SMB3 = 5;
constexpr static Byte CYC_INS_SMB4 = 5;
constexpr static Byte CYC_INS_SMB5 = 5;
constexpr static Byte CYC_INS_SMB6 = 5;
constexpr static Byte CYC_INS_SMB7 = 5;

constexpr static Byte CYC_INS_BBR0 = 5;
constexpr static Byte CYC_INS_BBR1 = 5;
constexpr static Byte CYC_INS_BBR2 = 5;
constexpr static Byte CYC_INS_BBR3 = 5;
constexpr static Byte CYC_INS_BBR4 = 5;
constexpr static Byte CYC_INS_BBR5 = 5;
constexpr static Byte CYC_INS_BBR6 = 5;
constexpr static Byte CYC_INS_BBR7 = 5;

constexpr static Byte CYC_INS_BBS0 = 5;
constexpr static Byte CYC_INS_BBS1 = 5;
constexpr static Byte CYC_INS_BBS2 = 5;
constexpr static Byte CYC_INS_BBS3 = 5;
constexpr static Byte CYC_INS_BBS4 = 5;
constexpr static Byte CYC_INS_BBS5 = 5;
constexpr static Byte CYC_INS_BBS6 = 5;
constexpr static Byte CYC_INS_BBS7 = 5;

constexpr static Byte CYC_INS_TRB_ABS = 6;
constexpr static Byte CYC_INS_TRB_ZP = 5;

constexpr static Byte CYC_INS_TSB_ABS = 6;
constexpr static Byte CYC_INS_TSB_ZP = 5;

enum class AddressingMode : std::uint8_t {
    Implied,
    Immediate,
    ZeroPage,
    ZeroPageX,
    ZeroPageY,
    Absolute,
    AbsoluteX,
    AbsoluteY,
    Indirect,
    IndirectX,
    IndirectY,
    Relative,
    Accumulator,
    ZeroPageIndirect,
    AbsoluteIndexedIndirect,  // JMP (abs,x)
    ZeroPageRelative          // BBR/BBS
};

struct InstructionInfo {
    const char* mnemonic;
    AddressingMode mode;
};

constexpr static std::array<InstructionInfo, 256> OpcodeTable = {{
    {.mnemonic = "BRK", .mode = AddressingMode::Implied},                  // 0x00
    {.mnemonic = "ORA", .mode = AddressingMode::IndirectX},                // 0x01
    {.mnemonic = "JAM", .mode = AddressingMode::Implied},                  // 0x02
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x03
    {.mnemonic = "TSB", .mode = AddressingMode::ZeroPage},                 // 0x04
    {.mnemonic = "ORA", .mode = AddressingMode::ZeroPage},                 // 0x05
    {.mnemonic = "ASL", .mode = AddressingMode::ZeroPage},                 // 0x06
    {.mnemonic = "RMB0", .mode = AddressingMode::ZeroPage},                // 0x07
    {.mnemonic = "PHP", .mode = AddressingMode::Implied},                  // 0x08
    {.mnemonic = "ORA", .mode = AddressingMode::Immediate},                // 0x09
    {.mnemonic = "ASL", .mode = AddressingMode::Accumulator},              // 0x0a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x0b
    {.mnemonic = "TSB", .mode = AddressingMode::Absolute},                 // 0x0c
    {.mnemonic = "ORA", .mode = AddressingMode::Absolute},                 // 0x0d
    {.mnemonic = "ASL", .mode = AddressingMode::Absolute},                 // 0x0e
    {.mnemonic = "BBR0", .mode = AddressingMode::ZeroPageRelative},        // 0x0f
    {.mnemonic = "BPL", .mode = AddressingMode::Relative},                 // 0x10
    {.mnemonic = "ORA", .mode = AddressingMode::IndirectY},                // 0x11
    {.mnemonic = "ORA", .mode = AddressingMode::ZeroPageIndirect},         // 0x12
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x13
    {.mnemonic = "TRB", .mode = AddressingMode::ZeroPage},                 // 0x14
    {.mnemonic = "ORA", .mode = AddressingMode::ZeroPageX},                // 0x15
    {.mnemonic = "ASL", .mode = AddressingMode::ZeroPageX},                // 0x16
    {.mnemonic = "RMB1", .mode = AddressingMode::ZeroPage},                // 0x17
    {.mnemonic = "CLC", .mode = AddressingMode::Implied},                  // 0x18
    {.mnemonic = "ORA", .mode = AddressingMode::AbsoluteY},                // 0x19
    {.mnemonic = "INC", .mode = AddressingMode::Accumulator},              // 0x1a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x1b
    {.mnemonic = "TRB", .mode = AddressingMode::Absolute},                 // 0x1c
    {.mnemonic = "ORA", .mode = AddressingMode::AbsoluteX},                // 0x1d
    {.mnemonic = "ASL", .mode = AddressingMode::AbsoluteX},                // 0x1e
    {.mnemonic = "BBR1", .mode = AddressingMode::ZeroPageRelative},        // 0x1f
    {.mnemonic = "JSR", .mode = AddressingMode::Absolute},                 // 0x20
    {.mnemonic = "AND", .mode = AddressingMode::IndirectX},                // 0x21
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x22
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x23
    {.mnemonic = "BIT", .mode = AddressingMode::ZeroPage},                 // 0x24
    {.mnemonic = "AND", .mode = AddressingMode::ZeroPage},                 // 0x25
    {.mnemonic = "ROL", .mode = AddressingMode::ZeroPage},                 // 0x26
    {.mnemonic = "RMB2", .mode = AddressingMode::ZeroPage},                // 0x27
    {.mnemonic = "PLP", .mode = AddressingMode::Implied},                  // 0x28
    {.mnemonic = "AND", .mode = AddressingMode::Immediate},                // 0x29
    {.mnemonic = "ROL", .mode = AddressingMode::Accumulator},              // 0x2a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x2b
    {.mnemonic = "BIT", .mode = AddressingMode::Absolute},                 // 0x2c
    {.mnemonic = "AND", .mode = AddressingMode::Absolute},                 // 0x2d
    {.mnemonic = "ROL", .mode = AddressingMode::Absolute},                 // 0x2e
    {.mnemonic = "BBR2", .mode = AddressingMode::ZeroPageRelative},        // 0x2f
    {.mnemonic = "BMI", .mode = AddressingMode::Relative},                 // 0x30
    {.mnemonic = "AND", .mode = AddressingMode::IndirectY},                // 0x31
    {.mnemonic = "AND", .mode = AddressingMode::ZeroPageIndirect},         // 0x32
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x33
    {.mnemonic = "BIT", .mode = AddressingMode::ZeroPageX},                // 0x34
    {.mnemonic = "AND", .mode = AddressingMode::ZeroPageX},                // 0x35
    {.mnemonic = "ROL", .mode = AddressingMode::ZeroPageX},                // 0x36
    {.mnemonic = "RMB3", .mode = AddressingMode::ZeroPage},                // 0x37
    {.mnemonic = "SEC", .mode = AddressingMode::Implied},                  // 0x38
    {.mnemonic = "AND", .mode = AddressingMode::AbsoluteY},                // 0x39
    {.mnemonic = "DEC", .mode = AddressingMode::Accumulator},              // 0x3a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x3b
    {.mnemonic = "BIT", .mode = AddressingMode::AbsoluteX},                // 0x3c
    {.mnemonic = "AND", .mode = AddressingMode::AbsoluteX},                // 0x3d
    {.mnemonic = "ROL", .mode = AddressingMode::AbsoluteX},                // 0x3e
    {.mnemonic = "BBR3", .mode = AddressingMode::ZeroPageRelative},        // 0x3f
    {.mnemonic = "RTI", .mode = AddressingMode::Implied},                  // 0x40
    {.mnemonic = "EOR", .mode = AddressingMode::IndirectX},                // 0x41
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x42
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x43
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x44
    {.mnemonic = "EOR", .mode = AddressingMode::ZeroPage},                 // 0x45
    {.mnemonic = "LSR", .mode = AddressingMode::ZeroPage},                 // 0x46
    {.mnemonic = "RMB4", .mode = AddressingMode::ZeroPage},                // 0x47
    {.mnemonic = "PHA", .mode = AddressingMode::Implied},                  // 0x48
    {.mnemonic = "EOR", .mode = AddressingMode::Immediate},                // 0x49
    {.mnemonic = "LSR", .mode = AddressingMode::Accumulator},              // 0x4a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x4b
    {.mnemonic = "JMP", .mode = AddressingMode::Absolute},                 // 0x4c
    {.mnemonic = "EOR", .mode = AddressingMode::Absolute},                 // 0x4d
    {.mnemonic = "LSR", .mode = AddressingMode::Absolute},                 // 0x4e
    {.mnemonic = "BBR4", .mode = AddressingMode::ZeroPageRelative},        // 0x4f
    {.mnemonic = "BVC", .mode = AddressingMode::Relative},                 // 0x50
    {.mnemonic = "EOR", .mode = AddressingMode::IndirectY},                // 0x51
    {.mnemonic = "EOR", .mode = AddressingMode::ZeroPageIndirect},         // 0x52
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x53
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x54
    {.mnemonic = "EOR", .mode = AddressingMode::ZeroPageX},                // 0x55
    {.mnemonic = "LSR", .mode = AddressingMode::ZeroPageX},                // 0x56
    {.mnemonic = "RMB5", .mode = AddressingMode::ZeroPage},                // 0x57
    {.mnemonic = "CLI", .mode = AddressingMode::Implied},                  // 0x58
    {.mnemonic = "EOR", .mode = AddressingMode::AbsoluteY},                // 0x59
    {.mnemonic = "PHY", .mode = AddressingMode::Implied},                  // 0x5a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x5b
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x5c
    {.mnemonic = "EOR", .mode = AddressingMode::AbsoluteX},                // 0x5d
    {.mnemonic = "LSR", .mode = AddressingMode::AbsoluteX},                // 0x5e
    {.mnemonic = "BBR5", .mode = AddressingMode::ZeroPageRelative},        // 0x5f
    {.mnemonic = "RTS", .mode = AddressingMode::Implied},                  // 0x60
    {.mnemonic = "ADC", .mode = AddressingMode::IndirectX},                // 0x61
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x62
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x63
    {.mnemonic = "STZ", .mode = AddressingMode::ZeroPage},                 // 0x64
    {.mnemonic = "ADC", .mode = AddressingMode::ZeroPage},                 // 0x65
    {.mnemonic = "ROR", .mode = AddressingMode::ZeroPage},                 // 0x66
    {.mnemonic = "RMB6", .mode = AddressingMode::ZeroPage},                // 0x67
    {.mnemonic = "PLA", .mode = AddressingMode::Implied},                  // 0x68
    {.mnemonic = "ADC", .mode = AddressingMode::Immediate},                // 0x69
    {.mnemonic = "ROR", .mode = AddressingMode::Accumulator},              // 0x6a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x6b
    {.mnemonic = "JMP", .mode = AddressingMode::Indirect},                 // 0x6c
    {.mnemonic = "ADC", .mode = AddressingMode::Absolute},                 // 0x6d
    {.mnemonic = "ROR", .mode = AddressingMode::Absolute},                 // 0x6e
    {.mnemonic = "BBR6", .mode = AddressingMode::ZeroPageRelative},        // 0x6f
    {.mnemonic = "BVS", .mode = AddressingMode::Relative},                 // 0x70
    {.mnemonic = "ADC", .mode = AddressingMode::IndirectY},                // 0x71
    {.mnemonic = "ADC", .mode = AddressingMode::ZeroPageIndirect},         // 0x72
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x73
    {.mnemonic = "STZ", .mode = AddressingMode::ZeroPageX},                // 0x74
    {.mnemonic = "ADC", .mode = AddressingMode::ZeroPageX},                // 0x75
    {.mnemonic = "ROR", .mode = AddressingMode::ZeroPageX},                // 0x76
    {.mnemonic = "RMB7", .mode = AddressingMode::ZeroPage},                // 0x77
    {.mnemonic = "SEI", .mode = AddressingMode::Implied},                  // 0x78
    {.mnemonic = "ADC", .mode = AddressingMode::AbsoluteY},                // 0x79
    {.mnemonic = "PLY", .mode = AddressingMode::Implied},                  // 0x7a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x7b
    {.mnemonic = "JMP", .mode = AddressingMode::AbsoluteIndexedIndirect},  // 0x7c
    {.mnemonic = "ADC", .mode = AddressingMode::AbsoluteX},                // 0x7d
    {.mnemonic = "ROR", .mode = AddressingMode::AbsoluteX},                // 0x7e
    {.mnemonic = "BBR7", .mode = AddressingMode::ZeroPageRelative},        // 0x7f
    {.mnemonic = "BRA", .mode = AddressingMode::Relative},                 // 0x80
    {.mnemonic = "STA", .mode = AddressingMode::IndirectX},                // 0x81
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x82
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x83
    {.mnemonic = "STY", .mode = AddressingMode::ZeroPage},                 // 0x84
    {.mnemonic = "STA", .mode = AddressingMode::ZeroPage},                 // 0x85
    {.mnemonic = "STX", .mode = AddressingMode::ZeroPage},                 // 0x86
    {.mnemonic = "SMB0", .mode = AddressingMode::ZeroPage},                // 0x87
    {.mnemonic = "DEY", .mode = AddressingMode::Implied},                  // 0x88
    {.mnemonic = "BIT", .mode = AddressingMode::Immediate},                // 0x89
    {.mnemonic = "TXA", .mode = AddressingMode::Implied},                  // 0x8a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x8b
    {.mnemonic = "STY", .mode = AddressingMode::Absolute},                 // 0x8c
    {.mnemonic = "STA", .mode = AddressingMode::Absolute},                 // 0x8d
    {.mnemonic = "STX", .mode = AddressingMode::Absolute},                 // 0x8e
    {.mnemonic = "BBS0", .mode = AddressingMode::ZeroPageRelative},        // 0x8f
    {.mnemonic = "BCC", .mode = AddressingMode::Relative},                 // 0x90
    {.mnemonic = "STA", .mode = AddressingMode::IndirectY},                // 0x91
    {.mnemonic = "STA", .mode = AddressingMode::ZeroPageIndirect},         // 0x92
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x93
    {.mnemonic = "STY", .mode = AddressingMode::ZeroPageX},                // 0x94
    {.mnemonic = "STA", .mode = AddressingMode::ZeroPageX},                // 0x95
    {.mnemonic = "STX", .mode = AddressingMode::ZeroPageY},                // 0x96
    {.mnemonic = "SMB1", .mode = AddressingMode::ZeroPage},                // 0x97
    {.mnemonic = "TYA", .mode = AddressingMode::Implied},                  // 0x98
    {.mnemonic = "STA", .mode = AddressingMode::AbsoluteY},                // 0x99
    {.mnemonic = "TXS", .mode = AddressingMode::Implied},                  // 0x9a
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0x9b
    {.mnemonic = "STZ", .mode = AddressingMode::Absolute},                 // 0x9c
    {.mnemonic = "STA", .mode = AddressingMode::AbsoluteX},                // 0x9d
    {.mnemonic = "STZ", .mode = AddressingMode::AbsoluteX},                // 0x9e
    {.mnemonic = "BBS1", .mode = AddressingMode::ZeroPageRelative},        // 0x9f
    {.mnemonic = "LDY", .mode = AddressingMode::Immediate},                // 0xa0
    {.mnemonic = "LDA", .mode = AddressingMode::IndirectX},                // 0xa1
    {.mnemonic = "LDX", .mode = AddressingMode::Immediate},                // 0xa2
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xa3
    {.mnemonic = "LDY", .mode = AddressingMode::ZeroPage},                 // 0xa4
    {.mnemonic = "LDA", .mode = AddressingMode::ZeroPage},                 // 0xa5
    {.mnemonic = "LDX", .mode = AddressingMode::ZeroPage},                 // 0xa6
    {.mnemonic = "SMB2", .mode = AddressingMode::ZeroPage},                // 0xa7
    {.mnemonic = "TAY", .mode = AddressingMode::Implied},                  // 0xa8
    {.mnemonic = "LDA", .mode = AddressingMode::Immediate},                // 0xa9
    {.mnemonic = "TAX", .mode = AddressingMode::Implied},                  // 0xaa
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xab
    {.mnemonic = "LDY", .mode = AddressingMode::Absolute},                 // 0xac
    {.mnemonic = "LDA", .mode = AddressingMode::Absolute},                 // 0xad
    {.mnemonic = "LDX", .mode = AddressingMode::Absolute},                 // 0xae
    {.mnemonic = "BBS2", .mode = AddressingMode::ZeroPageRelative},        // 0xaf
    {.mnemonic = "BCS", .mode = AddressingMode::Relative},                 // 0xb0
    {.mnemonic = "LDA", .mode = AddressingMode::IndirectY},                // 0xb1
    {.mnemonic = "LDA", .mode = AddressingMode::ZeroPageIndirect},         // 0xb2
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xb3
    {.mnemonic = "LDY", .mode = AddressingMode::ZeroPageX},                // 0xb4
    {.mnemonic = "LDA", .mode = AddressingMode::ZeroPageX},                // 0xb5
    {.mnemonic = "LDX", .mode = AddressingMode::ZeroPageY},                // 0xb6
    {.mnemonic = "SMB3", .mode = AddressingMode::ZeroPage},                // 0xb7
    {.mnemonic = "CLV", .mode = AddressingMode::Implied},                  // 0xb8
    {.mnemonic = "LDA", .mode = AddressingMode::AbsoluteY},                // 0xb9
    {.mnemonic = "TSX", .mode = AddressingMode::Implied},                  // 0xba
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xbb
    {.mnemonic = "LDY", .mode = AddressingMode::AbsoluteX},                // 0xbc
    {.mnemonic = "LDA", .mode = AddressingMode::AbsoluteX},                // 0xbd
    {.mnemonic = "LDX", .mode = AddressingMode::AbsoluteY},                // 0xbe
    {.mnemonic = "BBS3", .mode = AddressingMode::ZeroPageRelative},        // 0xbf
    {.mnemonic = "CPY", .mode = AddressingMode::Immediate},                // 0xc0
    {.mnemonic = "CMP", .mode = AddressingMode::IndirectX},                // 0xc1
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xc2
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xc3
    {.mnemonic = "CPY", .mode = AddressingMode::ZeroPage},                 // 0xc4
    {.mnemonic = "CMP", .mode = AddressingMode::ZeroPage},                 // 0xc5
    {.mnemonic = "DEC", .mode = AddressingMode::ZeroPage},                 // 0xc6
    {.mnemonic = "SMB4", .mode = AddressingMode::ZeroPage},                // 0xc7
    {.mnemonic = "INY", .mode = AddressingMode::Implied},                  // 0xc8
    {.mnemonic = "CMP", .mode = AddressingMode::Immediate},                // 0xc9
    {.mnemonic = "DEX", .mode = AddressingMode::Implied},                  // 0xca
    {.mnemonic = "WAI", .mode = AddressingMode::Implied},                  // 0xcb
    {.mnemonic = "CPY", .mode = AddressingMode::Absolute},                 // 0xcc
    {.mnemonic = "CMP", .mode = AddressingMode::Absolute},                 // 0xcd
    {.mnemonic = "DEC", .mode = AddressingMode::Absolute},                 // 0xce
    {.mnemonic = "BBS4", .mode = AddressingMode::ZeroPageRelative},        // 0xcf
    {.mnemonic = "BNE", .mode = AddressingMode::Relative},                 // 0xd0
    {.mnemonic = "CMP", .mode = AddressingMode::IndirectY},                // 0xd1
    {.mnemonic = "CMP", .mode = AddressingMode::ZeroPageIndirect},         // 0xd2
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xd3
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xd4
    {.mnemonic = "CMP", .mode = AddressingMode::ZeroPageX},                // 0xd5
    {.mnemonic = "DEC", .mode = AddressingMode::ZeroPageX},                // 0xd6
    {.mnemonic = "SMB5", .mode = AddressingMode::ZeroPage},                // 0xd7
    {.mnemonic = "CLD", .mode = AddressingMode::Implied},                  // 0xd8
    {.mnemonic = "CMP", .mode = AddressingMode::AbsoluteY},                // 0xd9
    {.mnemonic = "PHX", .mode = AddressingMode::Implied},                  // 0xda
    {.mnemonic = "STP", .mode = AddressingMode::Implied},                  // 0xdb
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xdc
    {.mnemonic = "CMP", .mode = AddressingMode::AbsoluteX},                // 0xdd
    {.mnemonic = "DEC", .mode = AddressingMode::AbsoluteX},                // 0xde
    {.mnemonic = "BBS5", .mode = AddressingMode::ZeroPageRelative},        // 0xdf
    {.mnemonic = "CPX", .mode = AddressingMode::Immediate},                // 0xe0
    {.mnemonic = "SBC", .mode = AddressingMode::IndirectX},                // 0xe1
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xe2
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xe3
    {.mnemonic = "CPX", .mode = AddressingMode::ZeroPage},                 // 0xe4
    {.mnemonic = "SBC", .mode = AddressingMode::ZeroPage},                 // 0xe5
    {.mnemonic = "INC", .mode = AddressingMode::ZeroPage},                 // 0xe6
    {.mnemonic = "SMB6", .mode = AddressingMode::ZeroPage},                // 0xe7
    {.mnemonic = "INX", .mode = AddressingMode::Implied},                  // 0xe8
    {.mnemonic = "SBC", .mode = AddressingMode::Immediate},                // 0xe9
    {.mnemonic = "NOP", .mode = AddressingMode::Implied},                  // 0xea
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xeb
    {.mnemonic = "CPX", .mode = AddressingMode::Absolute},                 // 0xec
    {.mnemonic = "SBC", .mode = AddressingMode::Absolute},                 // 0xed
    {.mnemonic = "INC", .mode = AddressingMode::Absolute},                 // 0xee
    {.mnemonic = "BBS6", .mode = AddressingMode::ZeroPageRelative},        // 0xef
    {.mnemonic = "BEQ", .mode = AddressingMode::Relative},                 // 0xf0
    {.mnemonic = "SBC", .mode = AddressingMode::IndirectY},                // 0xf1
    {.mnemonic = "SBC", .mode = AddressingMode::ZeroPageIndirect},         // 0xf2
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xf3
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xf4
    {.mnemonic = "SBC", .mode = AddressingMode::ZeroPageX},                // 0xf5
    {.mnemonic = "INC", .mode = AddressingMode::ZeroPageX},                // 0xf6
    {.mnemonic = "SMB7", .mode = AddressingMode::ZeroPage},                // 0xf7
    {.mnemonic = "SED", .mode = AddressingMode::Implied},                  // 0xf8
    {.mnemonic = "SBC", .mode = AddressingMode::AbsoluteY},                // 0xf9
    {.mnemonic = "PLX", .mode = AddressingMode::Implied},                  // 0xfa
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xfb
    {.mnemonic = "???", .mode = AddressingMode::Implied},                  // 0xfc
    {.mnemonic = "SBC", .mode = AddressingMode::AbsoluteX},                // 0xfd
    {.mnemonic = "INC", .mode = AddressingMode::AbsoluteX},                // 0xfe
    {.mnemonic = "BBS7", .mode = AddressingMode::ZeroPageRelative}         // 0xff
}};

}  // namespace Hardware
