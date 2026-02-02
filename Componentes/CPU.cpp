#include "CPU.h"

#include <iostream>

#include "../Instrucciones/ADC.h"
#include "../Instrucciones/AND.h"
#include "../Instrucciones/ASL.h"
#include "../Instrucciones/BCC.h"
#include "../Instrucciones/BCS.h"
#include "../Instrucciones/BEQ.h"
#include "../Instrucciones/BIT.h"
#include "../Instrucciones/BMI.h"
#include "../Instrucciones/BNE.h"
#include "../Instrucciones/BPL.h"
#include "../Instrucciones/BRA.h"
#include "../Instrucciones/BRK.h"
#include "../Instrucciones/BVC.h"
#include "../Instrucciones/BVS.h"
#include "../Instrucciones/CLC.h"
#include "../Instrucciones/CLD.h"
#include "../Instrucciones/CLI.h"
#include "../Instrucciones/CLV.h"
#include "../Instrucciones/CMP.h"
#include "../Instrucciones/CPX.h"
#include "../Instrucciones/CPY.h"
#include "../Instrucciones/DEC.h"
#include "../Instrucciones/DEX.h"
#include "../Instrucciones/DEY.h"
#include "../Instrucciones/EOR.h"
#include "../Instrucciones/INC.h"
#include "../Instrucciones/INX.h"
#include "../Instrucciones/INY.h"
#include "../Instrucciones/JMP.h"
#include "../Instrucciones/JSR.h"
#include "../Instrucciones/LDA.h"
#include "../Instrucciones/LDX.h"
#include "../Instrucciones/LDY.h"
#include "../Instrucciones/LSR.h"
#include "../Instrucciones/NOP.h"
#include "../Instrucciones/ORA.h"
#include "../Instrucciones/PHA.h"
#include "../Instrucciones/PHP.h"
#include "../Instrucciones/PHX.h"
#include "../Instrucciones/PHY.h"
#include "../Instrucciones/PLA.h"
#include "../Instrucciones/PLP.h"
#include "../Instrucciones/PLX.h"
#include "../Instrucciones/PLY.h"
#include "../Instrucciones/ROL.h"
#include "../Instrucciones/ROR.h"
#include "../Instrucciones/RTI.h"
#include "../Instrucciones/RTS.h"
#include "../Instrucciones/SBC.h"
#include "../Instrucciones/SEC.h"
#include "../Instrucciones/SED.h"
#include "../Instrucciones/SEI.h"
#include "../Instrucciones/STA.h"
#include "../Instrucciones/STX.h"
#include "../Instrucciones/STY.h"
#include "../Instrucciones/STZ.h"
#include "../Instrucciones/TAX.h"
#include "../Instrucciones/TAY.h"
#include "../Instrucciones/TSX.h"
#include "../Instrucciones/TXA.h"
#include "../Instrucciones/TXS.h"
#include "../Instrucciones/TYA.h"

void CPU::Ejecutar(Mem& mem) {
    while (true) {
        Byte opcode = FetchByte(mem);

        switch (opcode) {
            case INS_JAM: {
                return;
            }
            case INS_BRA: {
                BRA::Ejecutar(*this, mem);
                break;
            }
            case INS_NOP: {
                NOP::Ejecutar(*this, mem);
                break;
            }
            case INS_LDA_IM: {
                LDA::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_LDA_ZP: {
                LDA::EjecutarZP(*this, mem);
                break;
            }
            case INS_LDA_ZPX: {
                LDA::EjecutarZPX(*this, mem);
                break;
            }
            case INS_LDA_ABS: {
                LDA::EjecutarABS(*this, mem);
                break;
            }
            case INS_LDA_ABSX: {
                LDA::EjecutarABSX(*this, mem);
                break;
            }
            case INS_LDA_ABSY: {
                LDA::EjecutarABSY(*this, mem);
                break;
            }
            case INS_LDA_INDX: {
                LDA::EjecutarINDX(*this, mem);
                break;
            }
            case INS_LDA_INDY: {
                LDA::EjecutarINDY(*this, mem);
                break;
            }
            case INS_LDA_IND_ZP: {
                LDA::EjecutarIND_ZP(*this, mem);
                break;
            }
            case INS_JSR: {
                JSR::Ejecutar(*this, mem);
                break;
            }
            case INS_RTS: {
                RTS::Ejecutar(*this, mem);
                break;
            }
            case INS_LDX_IM: {
                LDX::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_LDX_ZP: {
                LDX::EjecutarZP(*this, mem);
                break;
            }
            case INS_LDX_ZPY: {
                LDX::EjecutarZPY(*this, mem);
                break;
            }
            case INS_LDX_ABS: {
                LDX::EjecutarABS(*this, mem);
                break;
            }
            case INS_LDX_ABSY: {
                LDX::EjecutarABSY(*this, mem);
                break;
            }
            case INS_LDY_IM: {
                LDY::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_LDY_ZP: {
                LDY::EjecutarZP(*this, mem);
                break;
            }
            case INS_LDY_ZPX: {
                LDY::EjecutarZPX(*this, mem);
                break;
            }
            case INS_LDY_ABS: {
                LDY::EjecutarABS(*this, mem);
                break;
            }
            case INS_LDY_ABSX: {
                LDY::EjecutarABSX(*this, mem);
                break;
            }
            case INS_JMP_ABS: {
                JMP::EjecutarABS(*this, mem);
                break;
            }
            case INS_JMP_IND: {
                JMP::EjecutarIND(*this, mem);
                break;
            }
            case INS_JMP_ABSX: {
                JMP::EjecutarABSX(*this, mem);
                break;
            }
            case INS_STA_ZP: {
                STA::EjecutarZP(*this, mem);
                break;
            }
            case INS_STA_ZPX: {
                STA::EjecutarZPX(*this, mem);
                break;
            }
            case INS_STA_ABS: {
                STA::EjecutarABS(*this, mem);
                break;
            }
            case INS_STA_ABSX: {
                STA::EjecutarABSX(*this, mem);
                break;
            }
            case INS_STA_ABSY: {
                STA::EjecutarABSY(*this, mem);
                break;
            }
            case INS_STA_INDX: {
                STA::EjecutarINDX(*this, mem);
                break;
            }
            case INS_STA_INDY: {
                STA::EjecutarINDY(*this, mem);
                break;
            }
            case INS_STA_IND_ZP: {
                STA::EjecutarINDZP(*this, mem);
                break;
            }
            case INS_STX_ZP: {
                STX::EjecutarZP(*this, mem);
                break;
            }
            case INS_STX_ZPY: {
                STX::EjecutarZPY(*this, mem);
                break;
            }
            case INS_STX_ABS: {
                STX::EjecutarABS(*this, mem);
                break;
            }
            case INS_STY_ZP: {
                STY::EjecutarZP(*this, mem);
                break;
            }
            case INS_STY_ZPX: {
                STY::EjecutarZPX(*this, mem);
                break;
            }
            case INS_STY_ABS: {
                STY::EjecutarABS(*this, mem);
                break;
            }
            case INS_STZ_ZP: {
                STZ::EjecutarZP(*this, mem);
                break;
            }
            case INS_STZ_ZPX: {
                STZ::EjecutarZPX(*this, mem);
                break;
            }
            case INS_STZ_ABS: {
                STZ::EjecutarABS(*this, mem);
                break;
            }
            case INS_STZ_ABSX: {
                STZ::EjecutarABSX(*this, mem);
                break;
            }
            case INS_TSX: {
                TSX::Ejecutar(*this, mem);
                break;
            }
            case INS_TXS: {
                TXS::Ejecutar(*this, mem);
                break;
            }
            case INS_PHA: {
                PHA::Ejecutar(*this, mem);
                break;
            }
            case INS_PHP: {
                PHP::Ejecutar(*this, mem);
                break;
            }
            case INS_PHX: {
                PHX::Ejecutar(*this, mem);
                break;
            }
            case INS_PHY: {
                PHY::Ejecutar(*this, mem);
                break;
            }
            case INS_PLA: {
                PLA::Ejecutar(*this, mem);
                break;
            }
            case INS_PLP: {
                PLP::Ejecutar(*this, mem);
                break;
            }
            case INS_PLX: {
                PLX::Ejecutar(*this, mem);
                break;
            }
            case INS_PLY: {
                PLY::Ejecutar(*this, mem);
                break;
            }
            case INS_TAX: {
                TAX::Ejecutar(*this, mem);
                break;
            }
            case INS_TXA: {
                TXA::Ejecutar(*this, mem);
                break;
            }
            case INS_TAY: {
                TAY::Ejecutar(*this, mem);
                break;
            }
            case INS_TYA: {
                TYA::Ejecutar(*this, mem);
                break;
            }
            case INS_AND_IM: {
                AND::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_AND_ZP: {
                AND::EjecutarZP(*this, mem);
                break;
            }
            case INS_AND_ZPX: {
                AND::EjecutarZPX(*this, mem);
                break;
            }
            case INS_AND_ABS: {
                AND::EjecutarABS(*this, mem);
                break;
            }
            case INS_AND_ABSX: {
                AND::EjecutarABSX(*this, mem);
                break;
            }
            case INS_AND_ABSY: {
                AND::EjecutarABSY(*this, mem);
                break;
            }
            case INS_AND_INDX: {
                AND::EjecutarINDX(*this, mem);
                break;
            }
            case INS_AND_INDY: {
                AND::EjecutarINDY(*this, mem);
                break;
            }
            case INS_AND_IND_ZP: {
                AND::EjecutarIND_ZP(*this, mem);
                break;
            }
            case INS_EOR_IM: {
                EOR::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_EOR_ZP: {
                EOR::EjecutarZP(*this, mem);
                break;
            }
            case INS_EOR_ZPX: {
                EOR::EjecutarZPX(*this, mem);
                break;
            }
            case INS_EOR_ABS: {
                EOR::EjecutarABS(*this, mem);
                break;
            }
            case INS_EOR_ABSX: {
                EOR::EjecutarABSX(*this, mem);
                break;
            }
            case INS_EOR_ABSY: {
                EOR::EjecutarABSY(*this, mem);
                break;
            }
            case INS_EOR_INDX: {
                EOR::EjecutarINDX(*this, mem);
                break;
            }
            case INS_EOR_INDY: {
                EOR::EjecutarINDY(*this, mem);
                break;
            }
            case INS_EOR_IND_ZP: {
                EOR::EjecutarIND_ZP(*this, mem);
                break;
            }
            case INS_ORA_IM: {
                ORA::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_ORA_ZP: {
                ORA::EjecutarZP(*this, mem);
                break;
            }
            case INS_ORA_ZPX: {
                ORA::EjecutarZPX(*this, mem);
                break;
            }
            case INS_ORA_ABS: {
                ORA::EjecutarABS(*this, mem);
                break;
            }
            case INS_ORA_ABSX: {
                ORA::EjecutarABSX(*this, mem);
                break;
            }
            case INS_ORA_ABSY: {
                ORA::EjecutarABSY(*this, mem);
                break;
            }
            case INS_ORA_INDX: {
                ORA::EjecutarINDX(*this, mem);
                break;
            }
            case INS_ORA_INDY: {
                ORA::EjecutarINDY(*this, mem);
                break;
            }
            case INS_ORA_IND_ZP: {
                ORA::EjecutarIND_ZP(*this, mem);
                break;
            }
            case INS_BIT_IM: {
                BIT::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_BIT_ZP: {
                BIT::EjecutarZP(*this, mem);
                break;
            }
            case INS_BIT_ZPX: {
                BIT::EjecutarZPX(*this, mem);
                break;
            }
            case INS_BIT_ABS: {
                BIT::EjecutarABS(*this, mem);
                break;
            }
            case INS_BIT_ABSX: {
                BIT::EjecutarABSX(*this, mem);
                break;
            }
            case INS_DEX: {
                DEX::Ejecutar(*this, mem);
                break;
            }
            case INS_DEY: {
                DEY::Ejecutar(*this, mem);
                break;
            }
            case INS_INX: {
                INX::Ejecutar(*this, mem);
                break;
            }
            case INS_INY: {
                INY::Ejecutar(*this, mem);
                break;
            }
            case INS_INC_A: {
                INC::EjecutarAcumulador(*this, mem);
                break;
            }
            case INS_INC_ZP: {
                INC::EjecutarZP(*this, mem);
                break;
            }
            case INS_INC_ZPX: {
                INC::EjecutarZPX(*this, mem);
                break;
            }
            case INS_INC_ABS: {
                INC::EjecutarABS(*this, mem);
                break;
            }
            case INS_INC_ABSX: {
                INC::EjecutarABSX(*this, mem);
                break;
            }
            case INS_DEC_A: {
                DEC::EjecutarAcumulador(*this, mem);
                break;
            }
            case INS_DEC_ZP: {
                DEC::EjecutarZP(*this, mem);
                break;
            }
            case INS_DEC_ZPX: {
                DEC::EjecutarZPX(*this, mem);
                break;
            }
            case INS_DEC_ABS: {
                DEC::EjecutarABS(*this, mem);
                break;
            }
            case INS_DEC_ABSX: {
                DEC::EjecutarABSX(*this, mem);
                break;
            }
            case INS_ADC_IM: {
                ADC::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_ADC_ZP: {
                ADC::EjecutarZP(*this, mem);
                break;
            }
            case INS_ADC_ZPX: {
                ADC::EjecutarZPX(*this, mem);
                break;
            }
            case INS_ADC_ABS: {
                ADC::EjecutarABS(*this, mem);
                break;
            }
            case INS_ADC_ABSX: {
                ADC::EjecutarABSX(*this, mem);
                break;
            }
            case INS_ADC_ABSY: {
                ADC::EjecutarABSY(*this, mem);
                break;
            }
            case INS_ADC_INDX: {
                ADC::EjecutarINDX(*this, mem);
                break;
            }
            case INS_ADC_INDY: {
                ADC::EjecutarINDY(*this, mem);
                break;
            }
            case INS_SBC_IM: {
                SBC::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_SBC_ZP: {
                SBC::EjecutarZP(*this, mem);
                break;
            }
            case INS_SBC_ZPX: {
                SBC::EjecutarZPX(*this, mem);
                break;
            }
            case INS_SBC_ABS: {
                SBC::EjecutarABS(*this, mem);
                break;
            }
            case INS_SBC_ABSX: {
                SBC::EjecutarABSX(*this, mem);
                break;
            }
            case INS_SBC_ABSY: {
                SBC::EjecutarABSY(*this, mem);
                break;
            }
            case INS_SBC_INDX: {
                SBC::EjecutarINDX(*this, mem);
                break;
            }
            case INS_SBC_INDY: {
                SBC::EjecutarINDY(*this, mem);
                break;
            }
            case INS_SBC_IND_ZP: {
                SBC::EjecutarIND_ZP(*this, mem);
                break;
            }
            case INS_CMP_IM: {
                CMP::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_CMP_ZP: {
                CMP::EjecutarZP(*this, mem);
                break;
            }
            case INS_CMP_ZPX: {
                CMP::EjecutarZPX(*this, mem);
                break;
            }
            case INS_CMP_ABS: {
                CMP::EjecutarABS(*this, mem);
                break;
            }
            case INS_CMP_ABSX: {
                CMP::EjecutarABSX(*this, mem);
                break;
            }
            case INS_CMP_ABSY: {
                CMP::EjecutarABSY(*this, mem);
                break;
            }
            case INS_CMP_INDX: {
                CMP::EjecutarINDX(*this, mem);
                break;
            }
            case INS_CMP_INDY: {
                CMP::EjecutarINDY(*this, mem);
                break;
            }
            case INS_CPX_IM: {
                CPX::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_CPX_ZP: {
                CPX::EjecutarZP(*this, mem);
                break;
            }
            case INS_CPX_ABS: {
                CPX::EjecutarABS(*this, mem);
                break;
            }
            case INS_CPY_IM: {
                CPY::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_CPY_ZP: {
                CPY::EjecutarZP(*this, mem);
                break;
            }
            case INS_CPY_ABS: {
                CPY::EjecutarABS(*this, mem);
                break;
            }
            case INS_ASL_A: {
                ASL::EjecutarAcumulador(*this, mem);
                break;
            }
            case INS_ASL_ZP: {
                ASL::EjecutarZP(*this, mem);
                break;
            }
            case INS_ASL_ZPX: {
                ASL::EjecutarZPX(*this, mem);
                break;
            }
            case INS_ASL_ABS: {
                ASL::EjecutarABS(*this, mem);
                break;
            }
            case INS_ASL_ABSX: {
                ASL::EjecutarABSX(*this, mem);
                break;
            }
            case INS_LSR_A: {
                LSR::EjecutarAcumulador(*this, mem);
                break;
            }
            case INS_LSR_ZP: {
                LSR::EjecutarZP(*this, mem);
                break;
            }
            case INS_LSR_ZPX: {
                LSR::EjecutarZPX(*this, mem);
                break;
            }
            case INS_LSR_ABS: {
                LSR::EjecutarABS(*this, mem);
                break;
            }
            case INS_LSR_ABSX: {
                LSR::EjecutarABSX(*this, mem);
                break;
            }
            case INS_ROL_A: {
                ROL::EjecutarAcumulador(*this, mem);
                break;
            }
            case INS_ROL_ZP: {
                ROL::EjecutarZP(*this, mem);
                break;
            }
            case INS_ROL_ZPX: {
                ROL::EjecutarZPX(*this, mem);
                break;
            }
            case INS_ROL_ABS: {
                ROL::EjecutarABS(*this, mem);
                break;
            }
            case INS_ROL_ABSX: {
                ROL::EjecutarABSX(*this, mem);
                break;
            }
            case INS_ROR_A: {
                ROR::EjecutarAcumulador(*this, mem);
                break;
            }
            case INS_ROR_ZP: {
                ROR::EjecutarZP(*this, mem);
                break;
            }
            case INS_ROR_ZPX: {
                ROR::EjecutarZPX(*this, mem);
                break;
            }
            case INS_ROR_ABS: {
                ROR::EjecutarABS(*this, mem);
                break;
            }
            case INS_ROR_ABSX: {
                ROR::EjecutarABSX(*this, mem);
                break;
            }
            case INS_BCC: {
                BCC::Ejecutar(*this, mem);
                break;
            }
            case INS_BRK: {
                BRK::Ejecutar(*this, mem);
                break;
            }
            case INS_BCS: {
                BCS::Ejecutar(*this, mem);
                break;
            }
            case INS_BNE: {
                BNE::Ejecutar(*this, mem);
                break;
            }
            case INS_BEQ: {
                BEQ::Ejecutar(*this, mem);
                break;
            }
            case INS_BMI: {
                BMI::Ejecutar(*this, mem);
                break;
            }
            case INS_BPL: {
                BPL::Ejecutar(*this, mem);
                break;
            }
            case INS_BVS: {
                BVS::Ejecutar(*this, mem);
                break;
            }
            case INS_BVC: {
                BVC::Ejecutar(*this, mem);
                break;
            }
            case INS_CLC: {
                CLC::Ejecutar(*this, mem);
                break;
            }
            case INS_CLI: {
                CLI::Ejecutar(*this, mem);
                break;
            }
            case INS_CLD: {
                CLD::Ejecutar(*this, mem);
                break;
            }
            case INS_SEC: {
                SEC::Ejecutar(*this, mem);
                break;
            }
            case INS_SEI: {
                SEI::Ejecutar(*this, mem);
                break;
            }
            case INS_SED: {
                SED::Ejecutar(*this, mem);
                break;
            }
            case INS_CLV: {
                CLV::Ejecutar(*this, mem);
                break;
            }
            case INS_RTI: {
                RTI::Ejecutar(*this, mem);
                break;
            }
            default:

#ifndef TESTING_ENV
                std::cout << "Opcode desconocido: 0x" << std::hex
                          << static_cast<int>(opcode) << " PC: 0x" << PC
                          << std::dec << " ejecución cancelada." << std::endl;
#endif

                return;
        }
    }
}

void CPU::PushByte(Byte val, Mem& mem) {
    mem[SP] = val;
    SP--;
}

Byte CPU::PopByte(Mem& mem) {
    SP++;
    return mem[SP];
}

void CPU::PushWord(Word val, Mem& mem) {
    PushByte((val >> 8) & 0xFF, mem);
    PushByte(val & 0xFF, mem);
}

Word CPU::PopWord(Mem& mem) {
    Word Low = PopByte(mem);
    Word High = PopByte(mem);
    return (High << 8) | Low;
}

void CPU::Reset(Mem& mem) {
    PC = 0xFFFC;  // Dirección de reinicio
    SP = 0x01FF;  // Inicio de pila (Top of Stack)

    // Reset de registros
    A = 0;
    X = 0;
    Y = 0;

    // Reset de flags
    C = 0;
    Z = 0;
    I = 0;
    D = 0;
    B = 0;
    V = 0;
    N = 0;

    mem.Init();
}

const Byte CPU::FetchByte(const Mem& mem) {
    Byte dato = mem[PC];
    PC++;
    return dato;
}

const Byte CPU::LeerByte(const Word dir, const Mem& mem) {
    Byte dato = mem[dir];
    return dato;
}

const Word CPU::FetchWord(const Mem& mem) {
    Byte Dato_Low = mem[PC];
    PC++;
    Byte Dato_High = mem[PC];
    PC++;
    Word dato = (Dato_High << 8) | Dato_Low;
    return dato;
}

const Word CPU::LeerWord(const Word dir, const Mem& mem) {
    Byte Dato_Low = mem[dir];
    Byte Dato_High = mem[dir + 1];
    Word dato = (Dato_High << 8) | Dato_Low;
    return dato;
}

const Byte CPU::GetStatus() const {
    Byte status = 0;
    status |= C;
    status |= Z << 1;
    status |= I << 2;
    status |= D << 3;
    status |= B << 4;
    status |= V << 6;
    status |= N << 7;
    return status;
}

void CPU::SetStatus(Byte status) {
    C = status & 0x01;
    Z = (status >> 1) & 0x01;
    I = (status >> 2) & 0x01;
    D = (status >> 3) & 0x01;
    B = (status >> 4) & 0x01;
    V = (status >> 6) & 0x01;
    N = (status >> 7) & 0x01;
}
