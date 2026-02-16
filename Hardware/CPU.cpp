#include "CPU.h"

#include <iostream>

#include "CPU/Instructions/ADC.h"
#include "CPU/Instructions/AND.h"
#include "CPU/Instructions/ASL.h"
#include "CPU/Instructions/BBR.h"
#include "CPU/Instructions/BBS.h"
#include "CPU/Instructions/BCC.h"
#include "CPU/Instructions/BCS.h"
#include "CPU/Instructions/BEQ.h"
#include "CPU/Instructions/BIT.h"
#include "CPU/Instructions/BMI.h"
#include "CPU/Instructions/BNE.h"
#include "CPU/Instructions/BPL.h"
#include "CPU/Instructions/BRA.h"
#include "CPU/Instructions/BRK.h"
#include "CPU/Instructions/BVC.h"
#include "CPU/Instructions/BVS.h"
#include "CPU/Instructions/CLC.h"
#include "CPU/Instructions/CLD.h"
#include "CPU/Instructions/CLI.h"
#include "CPU/Instructions/CLV.h"
#include "CPU/Instructions/CMP.h"
#include "CPU/Instructions/CPX.h"
#include "CPU/Instructions/CPY.h"
#include "CPU/Instructions/DEC.h"
#include "CPU/Instructions/DEX.h"
#include "CPU/Instructions/DEY.h"
#include "CPU/Instructions/EOR.h"
#include "CPU/Instructions/INC.h"
#include "CPU/Instructions/INX.h"
#include "CPU/Instructions/INY.h"
#include "CPU/Instructions/JMP.h"
#include "CPU/Instructions/JSR.h"
#include "CPU/Instructions/LDA.h"
#include "CPU/Instructions/LDX.h"
#include "CPU/Instructions/LDY.h"
#include "CPU/Instructions/LSR.h"
#include "CPU/Instructions/NOP.h"
#include "CPU/Instructions/ORA.h"
#include "CPU/Instructions/PHA.h"
#include "CPU/Instructions/PHP.h"
#include "CPU/Instructions/PHX.h"
#include "CPU/Instructions/PHY.h"
#include "CPU/Instructions/PLA.h"
#include "CPU/Instructions/PLP.h"
#include "CPU/Instructions/PLX.h"
#include "CPU/Instructions/PLY.h"
#include "CPU/Instructions/RMB.h"
#include "CPU/Instructions/ROL.h"
#include "CPU/Instructions/ROR.h"
#include "CPU/Instructions/RTI.h"
#include "CPU/Instructions/RTS.h"
#include "CPU/Instructions/SBC.h"
#include "CPU/Instructions/SEC.h"
#include "CPU/Instructions/SED.h"
#include "CPU/Instructions/SEI.h"
#include "CPU/Instructions/SMB.h"
#include "CPU/Instructions/STA.h"
#include "CPU/Instructions/STX.h"
#include "CPU/Instructions/STY.h"
#include "CPU/Instructions/STZ.h"
#include "CPU/Instructions/TAX.h"
#include "CPU/Instructions/TAY.h"
#include "CPU/Instructions/TRB.h"
#include "CPU/Instructions/TSB.h"
#include "CPU/Instructions/TSX.h"
#include "CPU/Instructions/TXA.h"
#include "CPU/Instructions/TXS.h"
#include "CPU/Instructions/TYA.h"

using namespace Hardware::Instructions;

namespace Hardware {

int CPU::Execute(Mem& mem) {
    while (true) {
        int res = Step(mem);
        if (res != 0) return res;
    }
    return 0;
}

void CPU::IRQ(Mem& mem) {
    waiting = false;
    if (!I) {
        PushWord(PC, mem);
        B = 0;
        PushByte(GetStatus(), mem);
        I = 1;
        D = 0;  // 65C02 clears Decimal flag in IRQ
        PC = ReadWord(0xFFFE, mem);
    }
}

int CPU::Step(Mem& mem) {
    if (!isInit) {
        PC = ReadWord(0xFFFC, mem);
        isInit = true;
    }

    // Check interrupts
    if (waiting) {
        if ((mem.Read(ACIA_STATUS) & 0x80) != 0) {
            waiting = false;
        } else {
            return 0;
        }
    }

    return Dispatch(mem);
}

int CPU::Dispatch(Mem& mem) {
    Byte opcode = FetchByte(mem);
    switch (opcode) {
        case INS_WAI: {
            waiting = true;
            return 0;
        }
        case INS_STP: {
            return 1;
        }
        case INS_JAM: {
            return 1;
        }
        case INS_BRA: {
            BRA::Execute(*this, mem);
            break;
        }
        case INS_NOP: {
            NOP::Execute(*this, mem);
            break;
        }
        case INS_LDA_IM: {
            LDA::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_LDA_ZP: {
            LDA::ExecuteZP(*this, mem);
            break;
        }
        case INS_LDA_ZPX: {
            LDA::ExecuteZPX(*this, mem);
            break;
        }
        case INS_LDA_ABS: {
            LDA::ExecuteABS(*this, mem);
            break;
        }
        case INS_LDA_ABSX: {
            LDA::ExecuteABSX(*this, mem);
            break;
        }
        case INS_LDA_ABSY: {
            LDA::ExecuteABSY(*this, mem);
            break;
        }
        case INS_LDA_INDX: {
            LDA::ExecuteINDX(*this, mem);
            break;
        }
        case INS_LDA_INDY: {
            LDA::ExecuteINDY(*this, mem);
            break;
        }
        case INS_LDA_IND_ZP: {
            LDA::ExecuteIND_ZP(*this, mem);
            break;
        }
        case INS_JSR: {
            JSR::Execute(*this, mem);
            break;
        }
        case INS_RTS: {
            RTS::Execute(*this, mem);
            break;
        }
        case INS_LDX_IM: {
            LDX::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_LDX_ZP: {
            LDX::ExecuteZP(*this, mem);
            break;
        }
        case INS_LDX_ZPY: {
            LDX::ExecuteZPY(*this, mem);
            break;
        }
        case INS_LDX_ABS: {
            LDX::ExecuteABS(*this, mem);
            break;
        }
        case INS_LDX_ABSY: {
            LDX::ExecuteABSY(*this, mem);
            break;
        }
        case INS_LDY_IM: {
            LDY::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_LDY_ZP: {
            LDY::ExecuteZP(*this, mem);
            break;
        }
        case INS_LDY_ZPX: {
            LDY::ExecuteZPX(*this, mem);
            break;
        }
        case INS_LDY_ABS: {
            LDY::ExecuteABS(*this, mem);
            break;
        }
        case INS_LDY_ABSX: {
            LDY::ExecuteABSX(*this, mem);
            break;
        }
        case INS_JMP_ABS: {
            JMP::ExecuteABS(*this, mem);
            break;
        }
        case INS_JMP_IND: {
            JMP::ExecuteIND(*this, mem);
            break;
        }
        case INS_JMP_ABSX: {
            JMP::ExecuteABSX(*this, mem);
            break;
        }
        case INS_STA_ZP: {
            STA::ExecuteZP(*this, mem);
            break;
        }
        case INS_STA_ZPX: {
            STA::ExecuteZPX(*this, mem);
            break;
        }
        case INS_STA_ABS: {
            STA::ExecuteABS(*this, mem);
            break;
        }
        case INS_STA_ABSX: {
            STA::ExecuteABSX(*this, mem);
            break;
        }
        case INS_STA_ABSY: {
            STA::ExecuteABSY(*this, mem);
            break;
        }
        case INS_STA_INDX: {
            STA::ExecuteINDX(*this, mem);
            break;
        }
        case INS_STA_INDY: {
            STA::ExecuteINDY(*this, mem);
            break;
        }
        case INS_STA_IND_ZP: {
            STA::ExecuteINDZP(*this, mem);
            break;
        }
        case INS_STX_ZP: {
            STX::ExecuteZP(*this, mem);
            break;
        }
        case INS_STX_ZPY: {
            STX::ExecuteZPY(*this, mem);
            break;
        }
        case INS_STX_ABS: {
            STX::ExecuteABS(*this, mem);
            break;
        }
        case INS_STY_ZP: {
            STY::ExecuteZP(*this, mem);
            break;
        }
        case INS_STY_ZPX: {
            STY::ExecuteZPX(*this, mem);
            break;
        }
        case INS_STY_ABS: {
            STY::ExecuteABS(*this, mem);
            break;
        }
        case INS_STZ_ZP: {
            STZ::ExecuteZP(*this, mem);
            break;
        }
        case INS_STZ_ZPX: {
            STZ::ExecuteZPX(*this, mem);
            break;
        }
        case INS_STZ_ABS: {
            STZ::ExecuteABS(*this, mem);
            break;
        }
        case INS_STZ_ABSX: {
            STZ::ExecuteABSX(*this, mem);
            break;
        }
        case INS_TSX: {
            TSX::Execute(*this, mem);
            break;
        }
        case INS_TXS: {
            TXS::Execute(*this, mem);
            break;
        }
        case INS_PHA: {
            PHA::Execute(*this, mem);
            break;
        }
        case INS_PHP: {
            PHP::Execute(*this, mem);
            break;
        }
        case INS_PHX: {
            PHX::Execute(*this, mem);
            break;
        }
        case INS_PHY: {
            PHY::Execute(*this, mem);
            break;
        }
        case INS_PLA: {
            PLA::Execute(*this, mem);
            break;
        }
        case INS_PLP: {
            PLP::Execute(*this, mem);
            break;
        }
        case INS_PLX: {
            PLX::Execute(*this, mem);
            break;
        }
        case INS_PLY: {
            PLY::Execute(*this, mem);
            break;
        }
        case INS_TAX: {
            TAX::Execute(*this, mem);
            break;
        }
        case INS_TXA: {
            TXA::Execute(*this, mem);
            break;
        }
        case INS_TAY: {
            TAY::Execute(*this, mem);
            break;
        }
        case INS_TYA: {
            TYA::Execute(*this, mem);
            break;
        }
        case INS_AND_IM: {
            AND::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_AND_ZP: {
            AND::ExecuteZP(*this, mem);
            break;
        }
        case INS_AND_ZPX: {
            AND::ExecuteZPX(*this, mem);
            break;
        }
        case INS_AND_ABS: {
            AND::ExecuteABS(*this, mem);
            break;
        }
        case INS_AND_ABSX: {
            AND::ExecuteABSX(*this, mem);
            break;
        }
        case INS_AND_ABSY: {
            AND::ExecuteABSY(*this, mem);
            break;
        }
        case INS_AND_INDX: {
            AND::ExecuteINDX(*this, mem);
            break;
        }
        case INS_AND_INDY: {
            AND::ExecuteINDY(*this, mem);
            break;
        }
        case INS_AND_IND_ZP: {
            AND::ExecuteIND_ZP(*this, mem);
            break;
        }
        case INS_EOR_IM: {
            EOR::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_EOR_ZP: {
            EOR::ExecuteZP(*this, mem);
            break;
        }
        case INS_EOR_ZPX: {
            EOR::ExecuteZPX(*this, mem);
            break;
        }
        case INS_EOR_ABS: {
            EOR::ExecuteABS(*this, mem);
            break;
        }
        case INS_EOR_ABSX: {
            EOR::ExecuteABSX(*this, mem);
            break;
        }
        case INS_EOR_ABSY: {
            EOR::ExecuteABSY(*this, mem);
            break;
        }
        case INS_EOR_INDX: {
            EOR::ExecuteINDX(*this, mem);
            break;
        }
        case INS_EOR_INDY: {
            EOR::ExecuteINDY(*this, mem);
            break;
        }
        case INS_EOR_IND_ZP: {
            EOR::ExecuteIND_ZP(*this, mem);
            break;
        }
        case INS_ORA_IM: {
            ORA::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_ORA_ZP: {
            ORA::ExecuteZP(*this, mem);
            break;
        }
        case INS_ORA_ZPX: {
            ORA::ExecuteZPX(*this, mem);
            break;
        }
        case INS_ORA_ABS: {
            ORA::ExecuteABS(*this, mem);
            break;
        }
        case INS_ORA_ABSX: {
            ORA::ExecuteABSX(*this, mem);
            break;
        }
        case INS_ORA_ABSY: {
            ORA::ExecuteABSY(*this, mem);
            break;
        }
        case INS_ORA_INDX: {
            ORA::ExecuteINDX(*this, mem);
            break;
        }
        case INS_ORA_INDY: {
            ORA::ExecuteINDY(*this, mem);
            break;
        }
        case INS_ORA_IND_ZP: {
            ORA::ExecuteIND_ZP(*this, mem);
            break;
        }
        case INS_BIT_IM: {
            BIT::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_BIT_ZP: {
            BIT::ExecuteZP(*this, mem);
            break;
        }
        case INS_BIT_ZPX: {
            BIT::ExecuteZPX(*this, mem);
            break;
        }
        case INS_BIT_ABS: {
            BIT::ExecuteABS(*this, mem);
            break;
        }
        case INS_BIT_ABSX: {
            BIT::ExecuteABSX(*this, mem);
            break;
        }
        case INS_DEX: {
            DEX::Execute(*this, mem);
            break;
        }
        case INS_DEY: {
            DEY::Execute(*this, mem);
            break;
        }
        case INS_INX: {
            INX::Execute(*this, mem);
            break;
        }
        case INS_INY: {
            INY::Execute(*this, mem);
            break;
        }
        case INS_INC_A: {
            INC::ExecuteAccumulator(*this, mem);
            break;
        }
        case INS_INC_ZP: {
            INC::ExecuteZP(*this, mem);
            break;
        }
        case INS_INC_ZPX: {
            INC::ExecuteZPX(*this, mem);
            break;
        }
        case INS_INC_ABS: {
            INC::ExecuteABS(*this, mem);
            break;
        }
        case INS_INC_ABSX: {
            INC::ExecuteABSX(*this, mem);
            break;
        }
        case INS_DEC_A: {
            DEC::ExecuteAccumulator(*this, mem);
            break;
        }
        case INS_DEC_ZP: {
            DEC::ExecuteZP(*this, mem);
            break;
        }
        case INS_DEC_ZPX: {
            DEC::ExecuteZPX(*this, mem);
            break;
        }
        case INS_DEC_ABS: {
            DEC::ExecuteABS(*this, mem);
            break;
        }
        case INS_DEC_ABSX: {
            DEC::ExecuteABSX(*this, mem);
            break;
        }
        case INS_ADC_IM: {
            ADC::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_ADC_ZP: {
            ADC::ExecuteZP(*this, mem);
            break;
        }
        case INS_ADC_ZPX: {
            ADC::ExecuteZPX(*this, mem);
            break;
        }
        case INS_ADC_ABS: {
            ADC::ExecuteABS(*this, mem);
            break;
        }
        case INS_ADC_ABSX: {
            ADC::ExecuteABSX(*this, mem);
            break;
        }
        case INS_ADC_ABSY: {
            ADC::ExecuteABSY(*this, mem);
            break;
        }
        case INS_ADC_INDX: {
            ADC::ExecuteINDX(*this, mem);
            break;
        }
        case INS_ADC_INDY: {
            ADC::ExecuteINDY(*this, mem);
            break;
        }
        case INS_ADC_IND_ZP: {
            ADC::ExecuteIND_ZP(*this, mem);
            break;
        }
        case INS_SBC_IM: {
            SBC::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_SBC_ZP: {
            SBC::ExecuteZP(*this, mem);
            break;
        }
        case INS_SBC_ZPX: {
            SBC::ExecuteZPX(*this, mem);
            break;
        }
        case INS_SBC_ABS: {
            SBC::ExecuteABS(*this, mem);
            break;
        }
        case INS_SBC_ABSX: {
            SBC::ExecuteABSX(*this, mem);
            break;
        }
        case INS_SBC_ABSY: {
            SBC::ExecuteABSY(*this, mem);
            break;
        }
        case INS_SBC_INDX: {
            SBC::ExecuteINDX(*this, mem);
            break;
        }
        case INS_SBC_INDY: {
            SBC::ExecuteINDY(*this, mem);
            break;
        }
        case INS_SBC_IND_ZP: {
            SBC::ExecuteIND_ZP(*this, mem);
            break;
        }
        case INS_CMP_IM: {
            CMP::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_CMP_ZP: {
            CMP::ExecuteZP(*this, mem);
            break;
        }
        case INS_CMP_ZPX: {
            CMP::ExecuteZPX(*this, mem);
            break;
        }
        case INS_CMP_ABS: {
            CMP::ExecuteABS(*this, mem);
            break;
        }
        case INS_CMP_ABSX: {
            CMP::ExecuteABSX(*this, mem);
            break;
        }
        case INS_CMP_ABSY: {
            CMP::ExecuteABSY(*this, mem);
            break;
        }
        case INS_CMP_INDX: {
            CMP::ExecuteINDX(*this, mem);
            break;
        }
        case INS_CMP_INDY: {
            CMP::ExecuteINDY(*this, mem);
            break;
        }
        case INS_CMP_IND_ZP: {
            CMP::ExecuteIND_ZP(*this, mem);
            break;
        }
        case INS_CPX_IM: {
            CPX::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_CPX_ZP: {
            CPX::ExecuteZP(*this, mem);
            break;
        }
        case INS_CPX_ABS: {
            CPX::ExecuteABS(*this, mem);
            break;
        }
        case INS_CPY_IM: {
            CPY::ExecuteImmediate(*this, mem);
            break;
        }
        case INS_CPY_ZP: {
            CPY::ExecuteZP(*this, mem);
            break;
        }
        case INS_CPY_ABS: {
            CPY::ExecuteABS(*this, mem);
            break;
        }
        case INS_ASL_A: {
            ASL::ExecuteAccumulator(*this, mem);
            break;
        }
        case INS_ASL_ZP: {
            ASL::ExecuteZP(*this, mem);
            break;
        }
        case INS_ASL_ZPX: {
            ASL::ExecuteZPX(*this, mem);
            break;
        }
        case INS_ASL_ABS: {
            ASL::ExecuteABS(*this, mem);
            break;
        }
        case INS_ASL_ABSX: {
            ASL::ExecuteABSX(*this, mem);
            break;
        }
        case INS_LSR_A: {
            LSR::ExecuteAccumulator(*this, mem);
            break;
        }
        case INS_LSR_ZP: {
            LSR::ExecuteZP(*this, mem);
            break;
        }
        case INS_LSR_ZPX: {
            LSR::ExecuteZPX(*this, mem);
            break;
        }
        case INS_LSR_ABS: {
            LSR::ExecuteABS(*this, mem);
            break;
        }
        case INS_LSR_ABSX: {
            LSR::ExecuteABSX(*this, mem);
            break;
        }
        case INS_ROL_A: {
            ROL::ExecuteAccumulator(*this, mem);
            break;
        }
        case INS_ROL_ZP: {
            ROL::ExecuteZP(*this, mem);
            break;
        }
        case INS_ROL_ZPX: {
            ROL::ExecuteZPX(*this, mem);
            break;
        }
        case INS_ROL_ABS: {
            ROL::ExecuteABS(*this, mem);
            break;
        }
        case INS_ROL_ABSX: {
            ROL::ExecuteABSX(*this, mem);
            break;
        }
        case INS_ROR_A: {
            ROR::ExecuteAccumulator(*this, mem);
            break;
        }
        case INS_ROR_ZP: {
            ROR::ExecuteZP(*this, mem);
            break;
        }
        case INS_ROR_ZPX: {
            ROR::ExecuteZPX(*this, mem);
            break;
        }
        case INS_ROR_ABS: {
            ROR::ExecuteABS(*this, mem);
            break;
        }
        case INS_ROR_ABSX: {
            ROR::ExecuteABSX(*this, mem);
            break;
        }
        case INS_BCC: {
            BCC::Execute(*this, mem);
            break;
        }
        case INS_BRK: {
            BRK::Execute(*this, mem);
            break;
        }
        case INS_BCS: {
            BCS::Execute(*this, mem);
            break;
        }
        case INS_BNE: {
            BNE::Execute(*this, mem);
            break;
        }
        case INS_BEQ: {
            BEQ::Execute(*this, mem);
            break;
        }
        case INS_BMI: {
            BMI::Execute(*this, mem);
            break;
        }
        case INS_BPL: {
            BPL::Execute(*this, mem);
            break;
        }
        case INS_BVS: {
            BVS::Execute(*this, mem);
            break;
        }
        case INS_BVC: {
            BVC::Execute(*this, mem);
            break;
        }
        case INS_CLC: {
            CLC::Execute(*this, mem);
            break;
        }
        case INS_CLI: {
            CLI::Execute(*this, mem);
            break;
        }
        case INS_CLD: {
            CLD::Execute(*this, mem);
            break;
        }
        case INS_SEC: {
            SEC::Execute(*this, mem);
            break;
        }
        case INS_SEI: {
            SEI::Execute(*this, mem);
            break;
        }
        case INS_SED: {
            SED::Execute(*this, mem);
            break;
        }
        case INS_CLV: {
            CLV::Execute(*this, mem);
            break;
        }
        case INS_RTI: {
            RTI::Execute(*this, mem);
            break;
        }
        case INS_RMB0: {
            RMB::Execute0(*this, mem);
            break;
        }
        case INS_RMB1: {
            RMB::Execute1(*this, mem);
            break;
        }
        case INS_RMB2: {
            RMB::Execute2(*this, mem);
            break;
        }
        case INS_RMB3: {
            RMB::Execute3(*this, mem);
            break;
        }
        case INS_RMB4: {
            RMB::Execute4(*this, mem);
            break;
        }
        case INS_RMB5: {
            RMB::Execute5(*this, mem);
            break;
        }
        case INS_RMB6: {
            RMB::Execute6(*this, mem);
            break;
        }
        case INS_RMB7: {
            RMB::Execute7(*this, mem);
            break;
        }
        case INS_SMB0: {
            SMB::Execute0(*this, mem);
            break;
        }
        case INS_SMB1: {
            SMB::Execute1(*this, mem);
            break;
        }
        case INS_SMB2: {
            SMB::Execute2(*this, mem);
            break;
        }
        case INS_SMB3: {
            SMB::Execute3(*this, mem);
            break;
        }
        case INS_SMB4: {
            SMB::Execute4(*this, mem);
            break;
        }
        case INS_SMB5: {
            SMB::Execute5(*this, mem);
            break;
        }
        case INS_SMB6: {
            SMB::Execute6(*this, mem);
            break;
        }
        case INS_SMB7: {
            SMB::Execute7(*this, mem);
            break;
        }
        case INS_BBR0: {
            BBR::Execute0(*this, mem);
            break;
        }
        case INS_BBR1: {
            BBR::Execute1(*this, mem);
            break;
        }
        case INS_BBR2: {
            BBR::Execute2(*this, mem);
            break;
        }
        case INS_BBR3: {
            BBR::Execute3(*this, mem);
            break;
        }
        case INS_BBR4: {
            BBR::Execute4(*this, mem);
            break;
        }
        case INS_BBR5: {
            BBR::Execute5(*this, mem);
            break;
        }
        case INS_BBR6: {
            BBR::Execute6(*this, mem);
            break;
        }
        case INS_BBR7: {
            BBR::Execute7(*this, mem);
            break;
        }
        case INS_BBS0: {
            BBS::Execute0(*this, mem);
            break;
        }
        case INS_BBS1: {
            BBS::Execute1(*this, mem);
            break;
        }
        case INS_BBS2: {
            BBS::Execute2(*this, mem);
            break;
        }
        case INS_BBS3: {
            BBS::Execute3(*this, mem);
            break;
        }
        case INS_BBS4: {
            BBS::Execute4(*this, mem);
            break;
        }
        case INS_BBS5: {
            BBS::Execute5(*this, mem);
            break;
        }
        case INS_BBS6: {
            BBS::Execute6(*this, mem);
            break;
        }
        case INS_BBS7: {
            BBS::Execute7(*this, mem);
            break;
        }
        case INS_TRB_ZP: {
            TRB::ExecuteZP(*this, mem);
            break;
        }
        case INS_TRB_ABS: {
            TRB::ExecuteABS(*this, mem);
            break;
        }
        case INS_TSB_ZP: {
            TSB::ExecuteZP(*this, mem);
            break;
        }
        case INS_TSB_ABS: {
            TSB::ExecuteABS(*this, mem);
            break;
        }
        default:
            std::cerr << "Unknown opcode: 0x" << std::hex
                      << static_cast<int>(opcode) << " PC: 0x" << PC << std::dec
                      << " execution cancelled." << std::endl;
            return -1;
    }
    return 0;
}

void CPU::PushByte(Byte val, Mem& mem) {
    mem.Write(SP, val);
    SP--;
    if (SP < 0x0100) SP = 0x01FF;  // Wrap a la cima de la pila
}

Byte CPU::PopByte(Mem& mem) {
    SP++;
    if (SP > 0x01FF) SP = 0x0100;  // Wrap a la base de la pila
    return mem.Read(SP);
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
    PC = 0xFFFC;
    SP = 0x01FF;  // Inicio de pila (Top of Stack)

    // Reset de registros
    A = 0;
    X = 0;
    Y = 0;

    // Reset flags
    C = 0;
    Z = 0;
    I = 0;
    D = 0;
    B = 0;
    V = 0;
    N = 0;
    isInit = false;
}

const Byte CPU::FetchByte(const Mem& mem) {
    Byte dato = mem[PC];
    PC++;
    return dato;
}

const Word CPU::FetchWord(const Mem& mem) {
    Word dato = mem[PC];
    dato |= (mem[PC + 1] << 8);
    PC += 2;
    return dato;
}

const Byte CPU::ReadByte(const Word addr, Mem& mem) { return mem.Read(addr); }

const Word CPU::ReadWord(const Word addr, Mem& mem) {
    Word dato = mem.Read(addr);
    dato |= (mem.Read(addr + 1) << 8);
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

}  // namespace Hardware
