#pragma once

#include <cstdint>

using Byte = uint8_t;
using Word = uint16_t;

namespace Hardware {

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

constexpr static Byte INS_ADC_IM = 0x69;    // ADC Immediate
constexpr static Byte INS_ADC_ZP = 0x65;    // ADC ZP
constexpr static Byte INS_ADC_ZPX = 0x75;   // ADC ZPX
constexpr static Byte INS_ADC_ABS = 0x6D;   // ADC ABS
constexpr static Byte INS_ADC_ABSX = 0x7D;  // ADC ABSX
constexpr static Byte INS_ADC_ABSY = 0x79;  // ADC ABSY
constexpr static Byte INS_ADC_INDX = 0x61;  // ADC INDX
constexpr static Byte INS_ADC_INDY = 0x71;  // ADC INDY
constexpr static Byte INS_ADC_IND_ZP = 0x72;  // ADC IND ZP

constexpr static Byte INS_SBC_IM = 0xE9;    // SBC Immediate
constexpr static Byte INS_SBC_ZP = 0xE5;    // SBC ZP
constexpr static Byte INS_SBC_ZPX = 0xF5;   // SBC ZPX
constexpr static Byte INS_SBC_ABS = 0xED;   // SBC ABS
constexpr static Byte INS_SBC_ABSX = 0xFD;  // SBC ABSX
constexpr static Byte INS_SBC_ABSY = 0xF9;  // SBC ABSY
constexpr static Byte INS_SBC_INDX = 0xE1;  // SBC INDX
constexpr static Byte INS_SBC_INDY = 0xF1;  // SBC INDY
constexpr static Byte INS_SBC_IND_ZP = 0xF2;  // SBC IND ZP

constexpr static Byte INS_CMP_IM = 0xC9;    // CMP Immediate
constexpr static Byte INS_CMP_ZP = 0xC5;    // CMP ZP
constexpr static Byte INS_CMP_ZPX = 0xD5;   // CMP ZPX
constexpr static Byte INS_CMP_ABS = 0xCD;   // CMP ABS
constexpr static Byte INS_CMP_ABSX = 0xDD;  // CMP ABSX
constexpr static Byte INS_CMP_ABSY = 0xD9;  // CMP ABSY
constexpr static Byte INS_CMP_INDX = 0xC1;  // CMP INDX
constexpr static Byte INS_CMP_INDY = 0xD1;  // CMP INDY
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

}  // namespace Hardware
