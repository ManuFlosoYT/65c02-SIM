#include "Dispatch.h"

#include <iostream>

#include "../CPU.h"
#include "Instructions/ADC.h"
#include "Instructions/AND.h"
#include "Instructions/ASL.h"
#include "Instructions/BBR.h"
#include "Instructions/BBS.h"
#include "Instructions/BCC.h"
#include "Instructions/BCS.h"
#include "Instructions/BEQ.h"
#include "Instructions/BIT.h"
#include "Instructions/BMI.h"
#include "Instructions/BNE.h"
#include "Instructions/BPL.h"
#include "Instructions/BRA.h"
#include "Instructions/BRK.h"
#include "Instructions/BVC.h"
#include "Instructions/BVS.h"
#include "Instructions/CLC.h"
#include "Instructions/CLD.h"
#include "Instructions/CLI.h"
#include "Instructions/CLV.h"
#include "Instructions/CMP.h"
#include "Instructions/CPX.h"
#include "Instructions/CPY.h"
#include "Instructions/DEC.h"
#include "Instructions/DEX.h"
#include "Instructions/DEY.h"
#include "Instructions/EOR.h"
#include "Instructions/INC.h"
#include "Instructions/INX.h"
#include "Instructions/INY.h"
#include "Instructions/JMP.h"
#include "Instructions/JSR.h"
#include "Instructions/LDA.h"
#include "Instructions/LDX.h"
#include "Instructions/LDY.h"
#include "Instructions/LSR.h"
#include "Instructions/NOP.h"
#include "Instructions/ORA.h"
#include "Instructions/PHA.h"
#include "Instructions/PHP.h"
#include "Instructions/PHX.h"
#include "Instructions/PHY.h"
#include "Instructions/PLA.h"
#include "Instructions/PLP.h"
#include "Instructions/PLX.h"
#include "Instructions/PLY.h"
#include "Instructions/RMB.h"
#include "Instructions/ROL.h"
#include "Instructions/ROR.h"
#include "Instructions/RTI.h"
#include "Instructions/RTS.h"
#include "Instructions/SBC.h"
#include "Instructions/SEC.h"
#include "Instructions/SED.h"
#include "Instructions/SEI.h"
#include "Instructions/SMB.h"
#include "Instructions/STA.h"
#include "Instructions/STX.h"
#include "Instructions/STY.h"
#include "Instructions/STZ.h"
#include "Instructions/TAX.h"
#include "Instructions/TAY.h"
#include "Instructions/TRB.h"
#include "Instructions/TSB.h"
#include "Instructions/TSX.h"
#include "Instructions/TXA.h"
#include "Instructions/TXS.h"
#include "Instructions/TYA.h"
#include "MemoryOps.h"

using namespace Hardware::Instructions;

namespace Hardware {
namespace CPUDispatch {

int Dispatch(CPU& cpu, Mem& mem) {
    Byte opcode = CPUMemoryOps::FetchByte(cpu, mem);
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
            BRA::Execute(cpu, mem);

            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BRA - 1;
            }
            break;
        }
        case INS_NOP: {
            NOP::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_NOP - 1;
            }
            break;
        }
        case INS_LDA_IM: {
            LDA::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_IM - 1;
            }
            break;
        }
        case INS_LDA_ZP: {
            LDA::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ZP - 1;
            }
            break;
        }
        case INS_LDA_ZPX: {
            LDA::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ZPX - 1;
            }
            break;
        }
        case INS_LDA_ABS: {
            LDA::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ABS - 1;
            }
            break;
        }
        case INS_LDA_ABSX: {
            LDA::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ABSX - 1;
            }
            break;
        }
        case INS_LDA_ABSY: {
            LDA::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_ABSY - 1;
            }
            break;
        }
        case INS_LDA_INDX: {
            LDA::ExecuteINDX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_INDX - 1;
            }
            break;
        }
        case INS_LDA_INDY: {
            LDA::ExecuteINDY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_INDY - 1;
            }
            break;
        }
        case INS_LDA_IND_ZP: {
            LDA::ExecuteIND_ZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDA_IND_ZP - 1;
            }
            break;
        }
        case INS_JSR: {
            JSR::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_JSR - 1;
            }
            break;
        }
        case INS_RTS: {
            RTS::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RTS - 1;
            }
            break;
        }
        case INS_LDX_IM: {
            LDX::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_IM - 1;
            }
            break;
        }
        case INS_LDX_ZP: {
            LDX::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_ZP - 1;
            }
            break;
        }
        case INS_LDX_ZPY: {
            LDX::ExecuteZPY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_ZPY - 1;
            }
            break;
        }
        case INS_LDX_ABS: {
            LDX::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_ABS - 1;
            }
            break;
        }
        case INS_LDX_ABSY: {
            LDX::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDX_ABSY - 1;
            }
            break;
        }
        case INS_LDY_IM: {
            LDY::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_IM - 1;
            }
            break;
        }
        case INS_LDY_ZP: {
            LDY::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_ZP - 1;
            }
            break;
        }
        case INS_LDY_ZPX: {
            LDY::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_ZPX - 1;
            }
            break;
        }
        case INS_LDY_ABS: {
            LDY::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_ABS - 1;
            }
            break;
        }
        case INS_LDY_ABSX: {
            LDY::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LDY_ABSX - 1;
            }
            break;
        }
        case INS_JMP_ABS: {
            JMP::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_JMP_ABS - 1;
            }
            break;
        }
        case INS_JMP_IND: {
            JMP::ExecuteIND(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_JMP_IND - 1;
            }
            break;
        }
        case INS_JMP_ABSX: {
            JMP::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_JMP_ABSX - 1;
            }
            break;
        }
        case INS_STA_ZP: {
            STA::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ZP - 1;
            }
            break;
        }
        case INS_STA_ZPX: {
            STA::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ZPX - 1;
            }
            break;
        }
        case INS_STA_ABS: {
            STA::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ABS - 1;
            }
            break;
        }
        case INS_STA_ABSX: {
            STA::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ABSX - 1;
            }
            break;
        }
        case INS_STA_ABSY: {
            STA::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_ABSY - 1;
            }
            break;
        }
        case INS_STA_INDX: {
            STA::ExecuteINDX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_INDX - 1;
            }
            break;
        }
        case INS_STA_INDY: {
            STA::ExecuteINDY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_INDY - 1;
            }
            break;
        }
        case INS_STA_IND_ZP: {
            STA::ExecuteINDZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STA_IND_ZP - 1;
            }
            break;
        }
        case INS_STX_ZP: {
            STX::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STX_ZP - 1;
            }
            break;
        }
        case INS_STX_ZPY: {
            STX::ExecuteZPY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STX_ZPY - 1;
            }
            break;
        }
        case INS_STX_ABS: {
            STX::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STX_ABS - 1;
            }
            break;
        }
        case INS_STY_ZP: {
            STY::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STY_ZP - 1;
            }
            break;
        }
        case INS_STY_ZPX: {
            STY::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STY_ZPX - 1;
            }
            break;
        }
        case INS_STY_ABS: {
            STY::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STY_ABS - 1;
            }
            break;
        }
        case INS_STZ_ZP: {
            STZ::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STZ_ZP - 1;
            }
            break;
        }
        case INS_STZ_ZPX: {
            STZ::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STZ_ZPX - 1;
            }
            break;
        }
        case INS_STZ_ABS: {
            STZ::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STZ_ABS - 1;
            }
            break;
        }
        case INS_STZ_ABSX: {
            STZ::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_STZ_ABSX - 1;
            }
            break;
        }
        case INS_TSX: {
            TSX::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TSX - 1;
            }
            break;
        }
        case INS_TXS: {
            TXS::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TXS - 1;
            }
            break;
        }
        case INS_PHA: {
            PHA::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PHA - 1;
            }
            break;
        }
        case INS_PHP: {
            PHP::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PHP - 1;
            }
            break;
        }
        case INS_PHX: {
            PHX::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PHX - 1;
            }
            break;
        }
        case INS_PHY: {
            PHY::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PHY - 1;
            }
            break;
        }
        case INS_PLA: {
            PLA::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PLA - 1;
            }
            break;
        }
        case INS_PLP: {
            PLP::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PLP - 1;
            }
            break;
        }
        case INS_PLX: {
            PLX::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PLX - 1;
            }
            break;
        }
        case INS_PLY: {
            PLY::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_PLY - 1;
            }
            break;
        }
        case INS_TAX: {
            TAX::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TAX - 1;
            }
            break;
        }
        case INS_TXA: {
            TXA::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TXA - 1;
            }
            break;
        }
        case INS_TAY: {
            TAY::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TAY - 1;
            }
            break;
        }
        case INS_TYA: {
            TYA::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TYA - 1;
            }
            break;
        }
        case INS_AND_IM: {
            AND::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_IM - 1;
            }
            break;
        }
        case INS_AND_ZP: {
            AND::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ZP - 1;
            }
            break;
        }
        case INS_AND_ZPX: {
            AND::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ZPX - 1;
            }
            break;
        }
        case INS_AND_ABS: {
            AND::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ABS - 1;
            }
            break;
        }
        case INS_AND_ABSX: {
            AND::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ABSX - 1;
            }
            break;
        }
        case INS_AND_ABSY: {
            AND::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_ABSY - 1;
            }
            break;
        }
        case INS_AND_INDX: {
            AND::ExecuteINDX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_INDX - 1;
            }
            break;
        }
        case INS_AND_INDY: {
            AND::ExecuteINDY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_INDY - 1;
            }
            break;
        }
        case INS_AND_IND_ZP: {
            AND::ExecuteIND_ZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_AND_IND_ZP - 1;
            }
            break;
        }
        case INS_EOR_IM: {
            EOR::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_IM - 1;
            }
            break;
        }
        case INS_EOR_ZP: {
            EOR::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ZP - 1;
            }
            break;
        }
        case INS_EOR_ZPX: {
            EOR::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ZPX - 1;
            }
            break;
        }
        case INS_EOR_ABS: {
            EOR::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ABS - 1;
            }
            break;
        }
        case INS_EOR_ABSX: {
            EOR::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ABSX - 1;
            }
            break;
        }
        case INS_EOR_ABSY: {
            EOR::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_ABSY - 1;
            }
            break;
        }
        case INS_EOR_INDX: {
            EOR::ExecuteINDX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_INDX - 1;
            }
            break;
        }
        case INS_EOR_INDY: {
            EOR::ExecuteINDY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_INDY - 1;
            }
            break;
        }
        case INS_EOR_IND_ZP: {
            EOR::ExecuteIND_ZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_EOR_IND_ZP - 1;
            }
            break;
        }
        case INS_ORA_IM: {
            ORA::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_IM - 1;
            }
            break;
        }
        case INS_ORA_ZP: {
            ORA::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ZP - 1;
            }
            break;
        }
        case INS_ORA_ZPX: {
            ORA::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ZPX - 1;
            }
            break;
        }
        case INS_ORA_ABS: {
            ORA::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ABS - 1;
            }
            break;
        }
        case INS_ORA_ABSX: {
            ORA::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ABSX - 1;
            }
            break;
        }
        case INS_ORA_ABSY: {
            ORA::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_ABSY - 1;
            }
            break;
        }
        case INS_ORA_INDX: {
            ORA::ExecuteINDX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_INDX - 1;
            }
            break;
        }
        case INS_ORA_INDY: {
            ORA::ExecuteINDY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_INDY - 1;
            }
            break;
        }
        case INS_ORA_IND_ZP: {
            ORA::ExecuteIND_ZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ORA_IND_ZP - 1;
            }
            break;
        }
        case INS_BIT_IM: {
            BIT::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_IM - 1;
            }
            break;
        }
        case INS_BIT_ZP: {
            BIT::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_ZP - 1;
            }
            break;
        }
        case INS_BIT_ZPX: {
            BIT::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_ZPX - 1;
            }
            break;
        }
        case INS_BIT_ABS: {
            BIT::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_ABS - 1;
            }
            break;
        }
        case INS_BIT_ABSX: {
            BIT::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BIT_ABSX - 1;
            }
            break;
        }
        case INS_DEX: {
            DEX::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEX - 1;
            }
            break;
        }
        case INS_DEY: {
            DEY::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEY - 1;
            }
            break;
        }
        case INS_INX: {
            INX::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INX - 1;
            }
            break;
        }
        case INS_INY: {
            INY::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INY - 1;
            }
            break;
        }
        case INS_INC_A: {
            INC::ExecuteAccumulator(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_A - 1;
            }
            break;
        }
        case INS_INC_ZP: {
            INC::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_ZP - 1;
            }
            break;
        }
        case INS_INC_ZPX: {
            INC::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_ZPX - 1;
            }
            break;
        }
        case INS_INC_ABS: {
            INC::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_ABS - 1;
            }
            break;
        }
        case INS_INC_ABSX: {
            INC::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_INC_ABSX - 1;
            }
            break;
        }
        case INS_DEC_A: {
            DEC::ExecuteAccumulator(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_A - 1;
            }
            break;
        }
        case INS_DEC_ZP: {
            DEC::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_ZP - 1;
            }
            break;
        }
        case INS_DEC_ZPX: {
            DEC::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_ZPX - 1;
            }
            break;
        }
        case INS_DEC_ABS: {
            DEC::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_ABS - 1;
            }
            break;
        }
        case INS_DEC_ABSX: {
            DEC::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_DEC_ABSX - 1;
            }
            break;
        }
        case INS_ADC_IM: {
            ADC::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_IM - 1;
            }
            break;
        }
        case INS_ADC_ZP: {
            ADC::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ZP - 1;
            }
            break;
        }
        case INS_ADC_ZPX: {
            ADC::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ZPX - 1;
            }
            break;
        }
        case INS_ADC_ABS: {
            ADC::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ABS - 1;
            }
            break;
        }
        case INS_ADC_ABSX: {
            ADC::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ABSX - 1;
            }
            break;
        }
        case INS_ADC_ABSY: {
            ADC::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_ABSY - 1;
            }
            break;
        }
        case INS_ADC_INDX: {
            ADC::ExecuteINDX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_INDX - 1;
            }
            break;
        }
        case INS_ADC_INDY: {
            ADC::ExecuteINDY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_INDY - 1;
            }
            break;
        }
        case INS_ADC_IND_ZP: {
            ADC::ExecuteIND_ZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ADC_IND_ZP - 1;
            }
            break;
        }
        case INS_SBC_IM: {
            SBC::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_IM - 1;
            }
            break;
        }
        case INS_SBC_ZP: {
            SBC::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ZP - 1;
            }
            break;
        }
        case INS_SBC_ZPX: {
            SBC::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ZPX - 1;
            }
            break;
        }
        case INS_SBC_ABS: {
            SBC::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ABS - 1;
            }
            break;
        }
        case INS_SBC_ABSX: {
            SBC::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ABSX - 1;
            }
            break;
        }
        case INS_SBC_ABSY: {
            SBC::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_ABSY - 1;
            }
            break;
        }
        case INS_SBC_INDX: {
            SBC::ExecuteINDX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_INDX - 1;
            }
            break;
        }
        case INS_SBC_INDY: {
            SBC::ExecuteINDY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_INDY - 1;
            }
            break;
        }
        case INS_SBC_IND_ZP: {
            SBC::ExecuteIND_ZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SBC_IND_ZP - 1;
            }
            break;
        }
        case INS_CMP_IM: {
            CMP::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_IM - 1;
            }
            break;
        }
        case INS_CMP_ZP: {
            CMP::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ZP - 1;
            }
            break;
        }
        case INS_CMP_ZPX: {
            CMP::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ZPX - 1;
            }
            break;
        }
        case INS_CMP_ABS: {
            CMP::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ABS - 1;
            }
            break;
        }
        case INS_CMP_ABSX: {
            CMP::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ABSX - 1;
            }
            break;
        }
        case INS_CMP_ABSY: {
            CMP::ExecuteABSY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_ABSY - 1;
            }
            break;
        }
        case INS_CMP_INDX: {
            CMP::ExecuteINDX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_INDX - 1;
            }
            break;
        }
        case INS_CMP_INDY: {
            CMP::ExecuteINDY(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_INDY - 1;
            }
            break;
        }
        case INS_CMP_IND_ZP: {
            CMP::ExecuteIND_ZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CMP_IND_ZP - 1;
            }
            break;
        }
        case INS_CPX_IM: {
            CPX::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPX_IM - 1;
            }
            break;
        }
        case INS_CPX_ZP: {
            CPX::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPX_ZP - 1;
            }
            break;
        }
        case INS_CPX_ABS: {
            CPX::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPX_ABS - 1;
            }
            break;
        }
        case INS_CPY_IM: {
            CPY::ExecuteImmediate(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPY_IM - 1;
            }
            break;
        }
        case INS_CPY_ZP: {
            CPY::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPY_ZP - 1;
            }
            break;
        }
        case INS_CPY_ABS: {
            CPY::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CPY_ABS - 1;
            }
            break;
        }
        case INS_ASL_A: {
            ASL::ExecuteAccumulator(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_A - 1;
            }
            break;
        }
        case INS_ASL_ZP: {
            ASL::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_ZP - 1;
            }
            break;
        }
        case INS_ASL_ZPX: {
            ASL::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_ZPX - 1;
            }
            break;
        }
        case INS_ASL_ABS: {
            ASL::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_ABS - 1;
            }
            break;
        }
        case INS_ASL_ABSX: {
            ASL::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ASL_ABSX - 1;
            }
            break;
        }
        case INS_LSR_A: {
            LSR::ExecuteAccumulator(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_A - 1;
            }
            break;
        }
        case INS_LSR_ZP: {
            LSR::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_ZP - 1;
            }
            break;
        }
        case INS_LSR_ZPX: {
            LSR::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_ZPX - 1;
            }
            break;
        }
        case INS_LSR_ABS: {
            LSR::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_ABS - 1;
            }
            break;
        }
        case INS_LSR_ABSX: {
            LSR::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_LSR_ABSX - 1;
            }
            break;
        }
        case INS_ROL_A: {
            ROL::ExecuteAccumulator(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_A - 1;
            }
            break;
        }
        case INS_ROL_ZP: {
            ROL::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_ZP - 1;
            }
            break;
        }
        case INS_ROL_ZPX: {
            ROL::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_ZPX - 1;
            }
            break;
        }
        case INS_ROL_ABS: {
            ROL::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_ABS - 1;
            }
            break;
        }
        case INS_ROL_ABSX: {
            ROL::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROL_ABSX - 1;
            }
            break;
        }
        case INS_ROR_A: {
            ROR::ExecuteAccumulator(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_A - 1;
            }
            break;
        }
        case INS_ROR_ZP: {
            ROR::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_ZP - 1;
            }
            break;
        }
        case INS_ROR_ZPX: {
            ROR::ExecuteZPX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_ZPX - 1;
            }
            break;
        }
        case INS_ROR_ABS: {
            ROR::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_ABS - 1;
            }
            break;
        }
        case INS_ROR_ABSX: {
            ROR::ExecuteABSX(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_ROR_ABSX - 1;
            }
            break;
        }
        case INS_BCC: {
            BCC::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BCC - 1;
            }
            break;
        }
        case INS_BRK: {
            BRK::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BRK - 1;
            }
            break;
        }
        case INS_BCS: {
            BCS::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BCS - 1;
            }
            break;
        }
        case INS_BNE: {
            BNE::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BNE - 1;
            }
            break;
        }
        case INS_BEQ: {
            BEQ::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BEQ - 1;
            }
            break;
        }
        case INS_BMI: {
            BMI::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BMI - 1;
            }
            break;
        }
        case INS_BPL: {
            BPL::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BPL - 1;
            }
            break;
        }
        case INS_BVS: {
            BVS::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BVS - 1;
            }
            break;
        }
        case INS_BVC: {
            BVC::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BVC - 1;
            }
            break;
        }
        case INS_CLC: {
            CLC::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CLC - 1;
            }
            break;
        }
        case INS_CLI: {
            CLI::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CLI - 1;
            }
            break;
        }
        case INS_CLD: {
            CLD::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CLD - 1;
            }
            break;
        }
        case INS_SEC: {
            SEC::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SEC - 1;
            }
            break;
        }
        case INS_SEI: {
            SEI::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SEI - 1;
            }
            break;
        }
        case INS_SED: {
            SED::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SED - 1;
            }
            break;
        }
        case INS_CLV: {
            CLV::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_CLV - 1;
            }
            break;
        }
        case INS_RTI: {
            RTI::Execute(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RTI - 1;
            }
            break;
        }
        case INS_RMB0: {
            RMB::Execute0(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB0 - 1;
            }
            break;
        }
        case INS_RMB1: {
            RMB::Execute1(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB1 - 1;
            }
            break;
        }
        case INS_RMB2: {
            RMB::Execute2(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB2 - 1;
            }
            break;
        }
        case INS_RMB3: {
            RMB::Execute3(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB3 - 1;
            }
            break;
        }
        case INS_RMB4: {
            RMB::Execute4(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB4 - 1;
            }
            break;
        }
        case INS_RMB5: {
            RMB::Execute5(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB5 - 1;
            }
            break;
        }
        case INS_RMB6: {
            RMB::Execute6(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB6 - 1;
            }
            break;
        }
        case INS_RMB7: {
            RMB::Execute7(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_RMB7 - 1;
            }
            break;
        }
        case INS_SMB0: {
            SMB::Execute0(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB0 - 1;
            }
            break;
        }
        case INS_SMB1: {
            SMB::Execute1(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB1 - 1;
            }
            break;
        }
        case INS_SMB2: {
            SMB::Execute2(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB2 - 1;
            }
            break;
        }
        case INS_SMB3: {
            SMB::Execute3(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB3 - 1;
            }
            break;
        }
        case INS_SMB4: {
            SMB::Execute4(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB4 - 1;
            }
            break;
        }
        case INS_SMB5: {
            SMB::Execute5(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB5 - 1;
            }
            break;
        }
        case INS_SMB6: {
            SMB::Execute6(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB6 - 1;
            }
            break;
        }
        case INS_SMB7: {
            SMB::Execute7(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_SMB7 - 1;
            }
            break;
        }
        case INS_BBR0: {
            BBR::Execute0(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR0 - 1;
            }
            break;
        }
        case INS_BBR1: {
            BBR::Execute1(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR1 - 1;
            }
            break;
        }
        case INS_BBR2: {
            BBR::Execute2(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR2 - 1;
            }
            break;
        }
        case INS_BBR3: {
            BBR::Execute3(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR3 - 1;
            }
            break;
        }
        case INS_BBR4: {
            BBR::Execute4(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR4 - 1;
            }
            break;
        }
        case INS_BBR5: {
            BBR::Execute5(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR5 - 1;
            }
            break;
        }
        case INS_BBR6: {
            BBR::Execute6(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR6 - 1;
            }
            break;
        }
        case INS_BBR7: {
            BBR::Execute7(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBR7 - 1;
            }
            break;
        }
        case INS_BBS0: {
            BBS::Execute0(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS0 - 1;
            }
            break;
        }
        case INS_BBS1: {
            BBS::Execute1(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS1 - 1;
            }
            break;
        }
        case INS_BBS2: {
            BBS::Execute2(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS2 - 1;
            }
            break;
        }
        case INS_BBS3: {
            BBS::Execute3(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS3 - 1;
            }
            break;
        }
        case INS_BBS4: {
            BBS::Execute4(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS4 - 1;
            }
            break;
        }
        case INS_BBS5: {
            BBS::Execute5(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS5 - 1;
            }
            break;
        }
        case INS_BBS6: {
            BBS::Execute6(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS6 - 1;
            }
            break;
        }
        case INS_BBS7: {
            BBS::Execute7(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_BBS7 - 1;
            }
            break;
        }
        case INS_TRB_ZP: {
            TRB::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TRB_ZP - 1;
            }
            break;
        }
        case INS_TRB_ABS: {
            TRB::ExecuteABS(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TRB_ABS - 1;
            }
            break;
        }
        case INS_TSB_ZP: {
            TSB::ExecuteZP(cpu, mem);
            
            if (cpu.cycleAccurate) {
                cpu.remainingCycles = CYC_INS_TSB_ZP - 1;
            }
            break;
        }
        case INS_TSB_ABS: {
            TSB::ExecuteABS(cpu, mem);
            
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
