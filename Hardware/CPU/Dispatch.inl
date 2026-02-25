// Dispatch.inl — Inline implementations for Hardware::CPUDispatch
#include <iostream>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/ADC.h"
#include "Hardware/CPU/Instructions/AND.h"
#include "Hardware/CPU/Instructions/ASL.h"
#include "Hardware/CPU/Instructions/BBR.h"
#include "Hardware/CPU/Instructions/BBS.h"
#include "Hardware/CPU/Instructions/BCC.h"
#include "Hardware/CPU/Instructions/BCS.h"
#include "Hardware/CPU/Instructions/BEQ.h"
#include "Hardware/CPU/Instructions/BIT.h"
#include "Hardware/CPU/Instructions/BMI.h"
#include "Hardware/CPU/Instructions/BNE.h"
#include "Hardware/CPU/Instructions/BPL.h"
#include "Hardware/CPU/Instructions/BRA.h"
#include "Hardware/CPU/Instructions/BRK.h"
#include "Hardware/CPU/Instructions/BVC.h"
#include "Hardware/CPU/Instructions/BVS.h"
#include "Hardware/CPU/Instructions/CLC.h"
#include "Hardware/CPU/Instructions/CLD.h"
#include "Hardware/CPU/Instructions/CLI.h"
#include "Hardware/CPU/Instructions/CLV.h"
#include "Hardware/CPU/Instructions/CMP.h"
#include "Hardware/CPU/Instructions/CPX.h"
#include "Hardware/CPU/Instructions/CPY.h"
#include "Hardware/CPU/Instructions/DEC.h"
#include "Hardware/CPU/Instructions/DEX.h"
#include "Hardware/CPU/Instructions/DEY.h"
#include "Hardware/CPU/Instructions/EOR.h"
#include "Hardware/CPU/Instructions/INC.h"
#include "Hardware/CPU/Instructions/INX.h"
#include "Hardware/CPU/Instructions/INY.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/CPU/Instructions/JMP.h"
#include "Hardware/CPU/Instructions/JSR.h"
#include "Hardware/CPU/Instructions/LDA.h"
#include "Hardware/CPU/Instructions/LDX.h"
#include "Hardware/CPU/Instructions/LDY.h"
#include "Hardware/CPU/Instructions/LSR.h"
#include "Hardware/CPU/Instructions/NOP.h"
#include "Hardware/CPU/Instructions/ORA.h"
#include "Hardware/CPU/Instructions/PHA.h"
#include "Hardware/CPU/Instructions/PHP.h"
#include "Hardware/CPU/Instructions/PHX.h"
#include "Hardware/CPU/Instructions/PHY.h"
#include "Hardware/CPU/Instructions/PLA.h"
#include "Hardware/CPU/Instructions/PLP.h"
#include "Hardware/CPU/Instructions/PLX.h"
#include "Hardware/CPU/Instructions/PLY.h"
#include "Hardware/CPU/Instructions/RMB.h"
#include "Hardware/CPU/Instructions/ROL.h"
#include "Hardware/CPU/Instructions/ROR.h"
#include "Hardware/CPU/Instructions/RTI.h"
#include "Hardware/CPU/Instructions/RTS.h"
#include "Hardware/CPU/Instructions/SBC.h"
#include "Hardware/CPU/Instructions/SEC.h"
#include "Hardware/CPU/Instructions/SED.h"
#include "Hardware/CPU/Instructions/SEI.h"
#include "Hardware/CPU/Instructions/SMB.h"
#include "Hardware/CPU/Instructions/STA.h"
#include "Hardware/CPU/Instructions/STX.h"
#include "Hardware/CPU/Instructions/STY.h"
#include "Hardware/CPU/Instructions/STZ.h"
#include "Hardware/CPU/Instructions/TAX.h"
#include "Hardware/CPU/Instructions/TAY.h"
#include "Hardware/CPU/Instructions/TRB.h"
#include "Hardware/CPU/Instructions/TSB.h"
#include "Hardware/CPU/Instructions/TSX.h"
#include "Hardware/CPU/Instructions/TXA.h"
#include "Hardware/CPU/Instructions/TXS.h"
#include "Hardware/CPU/Instructions/TYA.h"

using namespace Hardware::Instructions;

namespace Hardware {
namespace CPUDispatch {

inline int Dispatch(CPU& cpu, Bus& bus) {
    Byte opcode = cpu.FetchByte(bus);
    switch (opcode) {
        case INS_WAI: {
            cpu.waiting = true;
            return 0;
        }
        case INS_STP: {
            return 1;
        }
        case INS_JAM: {
            return 1;
        }
        case INS_BRA: {
            BRA::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BRA - 1;
            }
            break;
        }
        case INS_NOP: {
            NOP::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_NOP - 1;
            }
            break;
        }
        case INS_LDA_IM: {
            LDA::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_IM - 1;
            }
            break;
        }
        case INS_LDA_ZP: {
            LDA::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ZP - 1;
            }
            break;
        }
        case INS_LDA_ZPX: {
            LDA::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ZPX - 1;
            }
            break;
        }
        case INS_LDA_ABS: {
            LDA::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ABS - 1;
            }
            break;
        }
        case INS_LDA_ABSX: {
            LDA::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ABSX - 1;
            }
            break;
        }
        case INS_LDA_ABSY: {
            LDA::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ABSY - 1;
            }
            break;
        }
        case INS_LDA_INDX: {
            LDA::ExecuteINDX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_INDX - 1;
            }
            break;
        }
        case INS_LDA_INDY: {
            LDA::ExecuteINDY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_INDY - 1;
            }
            break;
        }
        case INS_LDA_IND_ZP: {
            LDA::ExecuteIND_ZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_IND_ZP - 1;
            }
            break;
        }
        case INS_JSR: {
            JSR::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_JSR - 1;
            }
            break;
        }
        case INS_RTS: {
            RTS::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RTS - 1;
            }
            break;
        }
        case INS_LDX_IM: {
            LDX::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_IM - 1;
            }
            break;
        }
        case INS_LDX_ZP: {
            LDX::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_ZP - 1;
            }
            break;
        }
        case INS_LDX_ZPY: {
            LDX::ExecuteZPY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_ZPY - 1;
            }
            break;
        }
        case INS_LDX_ABS: {
            LDX::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_ABS - 1;
            }
            break;
        }
        case INS_LDX_ABSY: {
            LDX::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_ABSY - 1;
            }
            break;
        }
        case INS_LDY_IM: {
            LDY::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_IM - 1;
            }
            break;
        }
        case INS_LDY_ZP: {
            LDY::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_ZP - 1;
            }
            break;
        }
        case INS_LDY_ZPX: {
            LDY::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_ZPX - 1;
            }
            break;
        }
        case INS_LDY_ABS: {
            LDY::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_ABS - 1;
            }
            break;
        }
        case INS_LDY_ABSX: {
            LDY::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_ABSX - 1;
            }
            break;
        }
        case INS_JMP_ABS: {
            JMP::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_JMP_ABS - 1;
            }
            break;
        }
        case INS_JMP_IND: {
            JMP::ExecuteIND(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_JMP_IND - 1;
            }
            break;
        }
        case INS_JMP_ABSX: {
            JMP::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_JMP_ABSX - 1;
            }
            break;
        }
        case INS_STA_ZP: {
            STA::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ZP - 1;
            }
            break;
        }
        case INS_STA_ZPX: {
            STA::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ZPX - 1;
            }
            break;
        }
        case INS_STA_ABS: {
            STA::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ABS - 1;
            }
            break;
        }
        case INS_STA_ABSX: {
            STA::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ABSX - 1;
            }
            break;
        }
        case INS_STA_ABSY: {
            STA::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ABSY - 1;
            }
            break;
        }
        case INS_STA_INDX: {
            STA::ExecuteINDX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_INDX - 1;
            }
            break;
        }
        case INS_STA_INDY: {
            STA::ExecuteINDY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_INDY - 1;
            }
            break;
        }
        case INS_STA_IND_ZP: {
            STA::ExecuteINDZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_IND_ZP - 1;
            }
            break;
        }
        case INS_STX_ZP: {
            STX::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STX_ZP - 1;
            }
            break;
        }
        case INS_STX_ZPY: {
            STX::ExecuteZPY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STX_ZPY - 1;
            }
            break;
        }
        case INS_STX_ABS: {
            STX::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STX_ABS - 1;
            }
            break;
        }
        case INS_STY_ZP: {
            STY::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STY_ZP - 1;
            }
            break;
        }
        case INS_STY_ZPX: {
            STY::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STY_ZPX - 1;
            }
            break;
        }
        case INS_STY_ABS: {
            STY::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STY_ABS - 1;
            }
            break;
        }
        case INS_STZ_ZP: {
            STZ::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STZ_ZP - 1;
            }
            break;
        }
        case INS_STZ_ZPX: {
            STZ::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STZ_ZPX - 1;
            }
            break;
        }
        case INS_STZ_ABS: {
            STZ::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STZ_ABS - 1;
            }
            break;
        }
        case INS_STZ_ABSX: {
            STZ::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STZ_ABSX - 1;
            }
            break;
        }
        case INS_TSX: {
            TSX::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TSX - 1;
            }
            break;
        }
        case INS_TXS: {
            TXS::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TXS - 1;
            }
            break;
        }
        case INS_PHA: {
            PHA::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PHA - 1;
            }
            break;
        }
        case INS_PHP: {
            PHP::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PHP - 1;
            }
            break;
        }
        case INS_PHX: {
            PHX::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PHX - 1;
            }
            break;
        }
        case INS_PHY: {
            PHY::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PHY - 1;
            }
            break;
        }
        case INS_PLA: {
            PLA::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PLA - 1;
            }
            break;
        }
        case INS_PLP: {
            PLP::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PLP - 1;
            }
            break;
        }
        case INS_PLX: {
            PLX::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PLX - 1;
            }
            break;
        }
        case INS_PLY: {
            PLY::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PLY - 1;
            }
            break;
        }
        case INS_TAX: {
            TAX::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TAX - 1;
            }
            break;
        }
        case INS_TXA: {
            TXA::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TXA - 1;
            }
            break;
        }
        case INS_TAY: {
            TAY::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TAY - 1;
            }
            break;
        }
        case INS_TYA: {
            TYA::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TYA - 1;
            }
            break;
        }
        case INS_AND_IM: {
            AND::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_IM - 1;
            }
            break;
        }
        case INS_AND_ZP: {
            AND::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ZP - 1;
            }
            break;
        }
        case INS_AND_ZPX: {
            AND::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ZPX - 1;
            }
            break;
        }
        case INS_AND_ABS: {
            AND::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ABS - 1;
            }
            break;
        }
        case INS_AND_ABSX: {
            AND::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ABSX - 1;
            }
            break;
        }
        case INS_AND_ABSY: {
            AND::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ABSY - 1;
            }
            break;
        }
        case INS_AND_INDX: {
            AND::ExecuteINDX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_INDX - 1;
            }
            break;
        }
        case INS_AND_INDY: {
            AND::ExecuteINDY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_INDY - 1;
            }
            break;
        }
        case INS_AND_IND_ZP: {
            AND::ExecuteIND_ZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_IND_ZP - 1;
            }
            break;
        }
        case INS_EOR_IM: {
            EOR::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_IM - 1;
            }
            break;
        }
        case INS_EOR_ZP: {
            EOR::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ZP - 1;
            }
            break;
        }
        case INS_EOR_ZPX: {
            EOR::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ZPX - 1;
            }
            break;
        }
        case INS_EOR_ABS: {
            EOR::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ABS - 1;
            }
            break;
        }
        case INS_EOR_ABSX: {
            EOR::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ABSX - 1;
            }
            break;
        }
        case INS_EOR_ABSY: {
            EOR::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ABSY - 1;
            }
            break;
        }
        case INS_EOR_INDX: {
            EOR::ExecuteINDX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_INDX - 1;
            }
            break;
        }
        case INS_EOR_INDY: {
            EOR::ExecuteINDY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_INDY - 1;
            }
            break;
        }
        case INS_EOR_IND_ZP: {
            EOR::ExecuteIND_ZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_IND_ZP - 1;
            }
            break;
        }
        case INS_ORA_IM: {
            ORA::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_IM - 1;
            }
            break;
        }
        case INS_ORA_ZP: {
            ORA::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ZP - 1;
            }
            break;
        }
        case INS_ORA_ZPX: {
            ORA::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ZPX - 1;
            }
            break;
        }
        case INS_ORA_ABS: {
            ORA::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ABS - 1;
            }
            break;
        }
        case INS_ORA_ABSX: {
            ORA::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ABSX - 1;
            }
            break;
        }
        case INS_ORA_ABSY: {
            ORA::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ABSY - 1;
            }
            break;
        }
        case INS_ORA_INDX: {
            ORA::ExecuteINDX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_INDX - 1;
            }
            break;
        }
        case INS_ORA_INDY: {
            ORA::ExecuteINDY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_INDY - 1;
            }
            break;
        }
        case INS_ORA_IND_ZP: {
            ORA::ExecuteIND_ZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_IND_ZP - 1;
            }
            break;
        }
        case INS_BIT_IM: {
            BIT::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_IM - 1;
            }
            break;
        }
        case INS_BIT_ZP: {
            BIT::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_ZP - 1;
            }
            break;
        }
        case INS_BIT_ZPX: {
            BIT::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_ZPX - 1;
            }
            break;
        }
        case INS_BIT_ABS: {
            BIT::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_ABS - 1;
            }
            break;
        }
        case INS_BIT_ABSX: {
            BIT::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_ABSX - 1;
            }
            break;
        }
        case INS_DEX: {
            DEX::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEX - 1;
            }
            break;
        }
        case INS_DEY: {
            DEY::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEY - 1;
            }
            break;
        }
        case INS_INX: {
            INX::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INX - 1;
            }
            break;
        }
        case INS_INY: {
            INY::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INY - 1;
            }
            break;
        }
        case INS_INC_A: {
            INC::ExecuteAccumulator(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_A - 1;
            }
            break;
        }
        case INS_INC_ZP: {
            INC::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_ZP - 1;
            }
            break;
        }
        case INS_INC_ZPX: {
            INC::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_ZPX - 1;
            }
            break;
        }
        case INS_INC_ABS: {
            INC::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_ABS - 1;
            }
            break;
        }
        case INS_INC_ABSX: {
            INC::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_ABSX - 1;
            }
            break;
        }
        case INS_DEC_A: {
            DEC::ExecuteAccumulator(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_A - 1;
            }
            break;
        }
        case INS_DEC_ZP: {
            DEC::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_ZP - 1;
            }
            break;
        }
        case INS_DEC_ZPX: {
            DEC::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_ZPX - 1;
            }
            break;
        }
        case INS_DEC_ABS: {
            DEC::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_ABS - 1;
            }
            break;
        }
        case INS_DEC_ABSX: {
            DEC::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_ABSX - 1;
            }
            break;
        }
        case INS_ADC_IM: {
            ADC::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_IM - 1;
            }
            break;
        }
        case INS_ADC_ZP: {
            ADC::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ZP - 1;
            }
            break;
        }
        case INS_ADC_ZPX: {
            ADC::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ZPX - 1;
            }
            break;
        }
        case INS_ADC_ABS: {
            ADC::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ABS - 1;
            }
            break;
        }
        case INS_ADC_ABSX: {
            ADC::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ABSX - 1;
            }
            break;
        }
        case INS_ADC_ABSY: {
            ADC::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ABSY - 1;
            }
            break;
        }
        case INS_ADC_INDX: {
            ADC::ExecuteINDX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_INDX - 1;
            }
            break;
        }
        case INS_ADC_INDY: {
            ADC::ExecuteINDY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_INDY - 1;
            }
            break;
        }
        case INS_ADC_IND_ZP: {
            ADC::ExecuteIND_ZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_IND_ZP - 1;
            }
            break;
        }
        case INS_SBC_IM: {
            SBC::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_IM - 1;
            }
            break;
        }
        case INS_SBC_ZP: {
            SBC::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ZP - 1;
            }
            break;
        }
        case INS_SBC_ZPX: {
            SBC::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ZPX - 1;
            }
            break;
        }
        case INS_SBC_ABS: {
            SBC::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ABS - 1;
            }
            break;
        }
        case INS_SBC_ABSX: {
            SBC::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ABSX - 1;
            }
            break;
        }
        case INS_SBC_ABSY: {
            SBC::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ABSY - 1;
            }
            break;
        }
        case INS_SBC_INDX: {
            SBC::ExecuteINDX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_INDX - 1;
            }
            break;
        }
        case INS_SBC_INDY: {
            SBC::ExecuteINDY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_INDY - 1;
            }
            break;
        }
        case INS_SBC_IND_ZP: {
            SBC::ExecuteIND_ZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_IND_ZP - 1;
            }
            break;
        }
        case INS_CMP_IM: {
            CMP::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_IM - 1;
            }
            break;
        }
        case INS_CMP_ZP: {
            CMP::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ZP - 1;
            }
            break;
        }
        case INS_CMP_ZPX: {
            CMP::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ZPX - 1;
            }
            break;
        }
        case INS_CMP_ABS: {
            CMP::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ABS - 1;
            }
            break;
        }
        case INS_CMP_ABSX: {
            CMP::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ABSX - 1;
            }
            break;
        }
        case INS_CMP_ABSY: {
            CMP::ExecuteABSY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ABSY - 1;
            }
            break;
        }
        case INS_CMP_INDX: {
            CMP::ExecuteINDX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_INDX - 1;
            }
            break;
        }
        case INS_CMP_INDY: {
            CMP::ExecuteINDY(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_INDY - 1;
            }
            break;
        }
        case INS_CMP_IND_ZP: {
            CMP::ExecuteIND_ZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_IND_ZP - 1;
            }
            break;
        }
        case INS_CPX_IM: {
            CPX::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPX_IM - 1;
            }
            break;
        }
        case INS_CPX_ZP: {
            CPX::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPX_ZP - 1;
            }
            break;
        }
        case INS_CPX_ABS: {
            CPX::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPX_ABS - 1;
            }
            break;
        }
        case INS_CPY_IM: {
            CPY::ExecuteImmediate(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPY_IM - 1;
            }
            break;
        }
        case INS_CPY_ZP: {
            CPY::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPY_ZP - 1;
            }
            break;
        }
        case INS_CPY_ABS: {
            CPY::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPY_ABS - 1;
            }
            break;
        }
        case INS_ASL_A: {
            ASL::ExecuteAccumulator(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_A - 1;
            }
            break;
        }
        case INS_ASL_ZP: {
            ASL::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_ZP - 1;
            }
            break;
        }
        case INS_ASL_ZPX: {
            ASL::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_ZPX - 1;
            }
            break;
        }
        case INS_ASL_ABS: {
            ASL::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_ABS - 1;
            }
            break;
        }
        case INS_ASL_ABSX: {
            ASL::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_ABSX - 1;
            }
            break;
        }
        case INS_LSR_A: {
            LSR::ExecuteAccumulator(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_A - 1;
            }
            break;
        }
        case INS_LSR_ZP: {
            LSR::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_ZP - 1;
            }
            break;
        }
        case INS_LSR_ZPX: {
            LSR::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_ZPX - 1;
            }
            break;
        }
        case INS_LSR_ABS: {
            LSR::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_ABS - 1;
            }
            break;
        }
        case INS_LSR_ABSX: {
            LSR::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_ABSX - 1;
            }
            break;
        }
        case INS_ROL_A: {
            ROL::ExecuteAccumulator(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_A - 1;
            }
            break;
        }
        case INS_ROL_ZP: {
            ROL::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_ZP - 1;
            }
            break;
        }
        case INS_ROL_ZPX: {
            ROL::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_ZPX - 1;
            }
            break;
        }
        case INS_ROL_ABS: {
            ROL::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_ABS - 1;
            }
            break;
        }
        case INS_ROL_ABSX: {
            ROL::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_ABSX - 1;
            }
            break;
        }
        case INS_ROR_A: {
            ROR::ExecuteAccumulator(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_A - 1;
            }
            break;
        }
        case INS_ROR_ZP: {
            ROR::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_ZP - 1;
            }
            break;
        }
        case INS_ROR_ZPX: {
            ROR::ExecuteZPX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_ZPX - 1;
            }
            break;
        }
        case INS_ROR_ABS: {
            ROR::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_ABS - 1;
            }
            break;
        }
        case INS_ROR_ABSX: {
            ROR::ExecuteABSX(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_ABSX - 1;
            }
            break;
        }
        case INS_BCC: {
            BCC::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BCC - 1;
            }
            break;
        }
        case INS_BRK: {
            BRK::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BRK - 1;
            }
            break;
        }
        case INS_BCS: {
            BCS::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BCS - 1;
            }
            break;
        }
        case INS_BNE: {
            BNE::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BNE - 1;
            }
            break;
        }
        case INS_BEQ: {
            BEQ::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BEQ - 1;
            }
            break;
        }
        case INS_BMI: {
            BMI::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BMI - 1;
            }
            break;
        }
        case INS_BPL: {
            BPL::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BPL - 1;
            }
            break;
        }
        case INS_BVS: {
            BVS::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BVS - 1;
            }
            break;
        }
        case INS_BVC: {
            BVC::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BVC - 1;
            }
            break;
        }
        case INS_CLC: {
            CLC::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CLC - 1;
            }
            break;
        }
        case INS_CLI: {
            CLI::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CLI - 1;
            }
            break;
        }
        case INS_CLD: {
            CLD::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CLD - 1;
            }
            break;
        }
        case INS_SEC: {
            SEC::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SEC - 1;
            }
            break;
        }
        case INS_SEI: {
            SEI::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SEI - 1;
            }
            break;
        }
        case INS_SED: {
            SED::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SED - 1;
            }
            break;
        }
        case INS_CLV: {
            CLV::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CLV - 1;
            }
            break;
        }
        case INS_RTI: {
            RTI::Execute(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RTI - 1;
            }
            break;
        }
        case INS_RMB0: {
            RMB::Execute0(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB0 - 1;
            }
            break;
        }
        case INS_RMB1: {
            RMB::Execute1(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB1 - 1;
            }
            break;
        }
        case INS_RMB2: {
            RMB::Execute2(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB2 - 1;
            }
            break;
        }
        case INS_RMB3: {
            RMB::Execute3(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB3 - 1;
            }
            break;
        }
        case INS_RMB4: {
            RMB::Execute4(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB4 - 1;
            }
            break;
        }
        case INS_RMB5: {
            RMB::Execute5(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB5 - 1;
            }
            break;
        }
        case INS_RMB6: {
            RMB::Execute6(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB6 - 1;
            }
            break;
        }
        case INS_RMB7: {
            RMB::Execute7(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB7 - 1;
            }
            break;
        }
        case INS_SMB0: {
            SMB::Execute0(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB0 - 1;
            }
            break;
        }
        case INS_SMB1: {
            SMB::Execute1(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB1 - 1;
            }
            break;
        }
        case INS_SMB2: {
            SMB::Execute2(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB2 - 1;
            }
            break;
        }
        case INS_SMB3: {
            SMB::Execute3(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB3 - 1;
            }
            break;
        }
        case INS_SMB4: {
            SMB::Execute4(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB4 - 1;
            }
            break;
        }
        case INS_SMB5: {
            SMB::Execute5(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB5 - 1;
            }
            break;
        }
        case INS_SMB6: {
            SMB::Execute6(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB6 - 1;
            }
            break;
        }
        case INS_SMB7: {
            SMB::Execute7(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB7 - 1;
            }
            break;
        }
        case INS_BBR0: {
            BBR::Execute0(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR0 - 1;
            }
            break;
        }
        case INS_BBR1: {
            BBR::Execute1(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR1 - 1;
            }
            break;
        }
        case INS_BBR2: {
            BBR::Execute2(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR2 - 1;
            }
            break;
        }
        case INS_BBR3: {
            BBR::Execute3(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR3 - 1;
            }
            break;
        }
        case INS_BBR4: {
            BBR::Execute4(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR4 - 1;
            }
            break;
        }
        case INS_BBR5: {
            BBR::Execute5(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR5 - 1;
            }
            break;
        }
        case INS_BBR6: {
            BBR::Execute6(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR6 - 1;
            }
            break;
        }
        case INS_BBR7: {
            BBR::Execute7(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR7 - 1;
            }
            break;
        }
        case INS_BBS0: {
            BBS::Execute0(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS0 - 1;
            }
            break;
        }
        case INS_BBS1: {
            BBS::Execute1(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS1 - 1;
            }
            break;
        }
        case INS_BBS2: {
            BBS::Execute2(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS2 - 1;
            }
            break;
        }
        case INS_BBS3: {
            BBS::Execute3(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS3 - 1;
            }
            break;
        }
        case INS_BBS4: {
            BBS::Execute4(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS4 - 1;
            }
            break;
        }
        case INS_BBS5: {
            BBS::Execute5(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS5 - 1;
            }
            break;
        }
        case INS_BBS6: {
            BBS::Execute6(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS6 - 1;
            }
            break;
        }
        case INS_BBS7: {
            BBS::Execute7(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS7 - 1;
            }
            break;
        }
        case INS_TRB_ZP: {
            TRB::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TRB_ZP - 1;
            }
            break;
        }
        case INS_TRB_ABS: {
            TRB::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TRB_ABS - 1;
            }
            break;
        }
        case INS_TSB_ZP: {
            TSB::ExecuteZP(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TSB_ZP - 1;
            }
            break;
        }
        case INS_TSB_ABS: {
            TSB::ExecuteABS(cpu, bus);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TSB_ABS - 1;
            }
            break;
        }
        default:
            std::cerr << "Unknown opcode: 0x" << std::hex
                      << static_cast<int>(opcode) << " PC: 0x" << cpu.PC
                      << std::dec << " execution cancelled." << std::endl;
            return -1;
    }
    return 0;
}

}  // namespace CPUDispatch
}  // namespace Hardware
