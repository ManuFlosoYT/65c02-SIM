#pragma once
#include <array>
#include "Hardware/CPU/Dispatch.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/CPU/Instructions/ADC.h"
#include "Hardware/CPU/Instructions/AND.h"
#include "Hardware/CPU/Instructions/ASL.h"
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
#include "Hardware/CPU/Instructions/ROL.h"
#include "Hardware/CPU/Instructions/ROR.h"
#include "Hardware/CPU/Instructions/RTI.h"
#include "Hardware/CPU/Instructions/RTS.h"
#include "Hardware/CPU/Instructions/SBC.h"
#include "Hardware/CPU/Instructions/SEC.h"
#include "Hardware/CPU/Instructions/SED.h"
#include "Hardware/CPU/Instructions/SEI.h"
#include "Hardware/CPU/Instructions/STA.h"
#include "Hardware/CPU/Instructions/STX.h"
#include "Hardware/CPU/Instructions/STY.h"
#include "Hardware/CPU/Instructions/STZ.h"
#include "Hardware/CPU/Instructions/TAX.h"
#include "Hardware/CPU/Instructions/TAY.h"
#include "Hardware/CPU/Instructions/TSX.h"
#include "Hardware/CPU/Instructions/TXA.h"
#include "Hardware/CPU/Instructions/TXS.h"
#include "Hardware/CPU/Instructions/TYA.h"
#include "Hardware/CPU/Instructions/BBR.h"
#include "Hardware/CPU/Instructions/BBS.h"
#include "Hardware/CPU/Instructions/RMB.h"
#include "Hardware/CPU/Instructions/SMB.h"
#include "Hardware/CPU/Instructions/TRB.h"
#include "Hardware/CPU/Instructions/TSB.h"

using namespace Hardware::Instructions;

namespace Hardware {
namespace CPUDispatch {

constexpr std::array<OpcodeEntry, 256> BuildDispatchTable() {
    std::array<OpcodeEntry, 256> table{};
    for (auto& entry : table) {
        entry = {nullptr, 0, -1};
    }

    table[INS_WAI] = {[](CPU& cpu, Bus&) { cpu.waiting = true; }, 0, 0};
    table[INS_STP] = {nullptr, 0, 1};
    table[INS_JAM] = {nullptr, 0, 1};
    table[INS_BRA] = {&BRA::Execute, CYC_INS_BRA, 0};
    table[INS_NOP] = {&NOP::Execute, CYC_INS_NOP, 0};
    table[INS_LDA_IM] = {&LDA::ExecuteImmediate, CYC_INS_LDA_IM, 0};
    table[INS_LDA_ZP] = {&LDA::ExecuteZP, CYC_INS_LDA_ZP, 0};
    table[INS_LDA_ZPX] = {&LDA::ExecuteZPX, CYC_INS_LDA_ZPX, 0};
    table[INS_LDA_ABS] = {&LDA::ExecuteABS, CYC_INS_LDA_ABS, 0};
    table[INS_LDA_ABSX] = {&LDA::ExecuteABSX, CYC_INS_LDA_ABSX, 0};
    table[INS_LDA_ABSY] = {&LDA::ExecuteABSY, CYC_INS_LDA_ABSY, 0};
    table[INS_LDA_INDX] = {&LDA::ExecuteINDX, CYC_INS_LDA_INDX, 0};
    table[INS_LDA_INDY] = {&LDA::ExecuteINDY, CYC_INS_LDA_INDY, 0};
    table[INS_LDA_IND_ZP] = {&LDA::ExecuteIND_ZP, CYC_INS_LDA_IND_ZP, 0};
    table[INS_JSR] = {&JSR::Execute, CYC_INS_JSR, 0};
    table[INS_RTS] = {&RTS::Execute, CYC_INS_RTS, 0};
    table[INS_LDX_IM] = {&LDX::ExecuteImmediate, CYC_INS_LDX_IM, 0};
    table[INS_LDX_ZP] = {&LDX::ExecuteZP, CYC_INS_LDX_ZP, 0};
    table[INS_LDX_ZPY] = {&LDX::ExecuteZPY, CYC_INS_LDX_ZPY, 0};
    table[INS_LDX_ABS] = {&LDX::ExecuteABS, CYC_INS_LDX_ABS, 0};
    table[INS_LDX_ABSY] = {&LDX::ExecuteABSY, CYC_INS_LDX_ABSY, 0};
    table[INS_LDY_IM] = {&LDY::ExecuteImmediate, CYC_INS_LDY_IM, 0};
    table[INS_LDY_ZP] = {&LDY::ExecuteZP, CYC_INS_LDY_ZP, 0};
    table[INS_LDY_ZPX] = {&LDY::ExecuteZPX, CYC_INS_LDY_ZPX, 0};
    table[INS_LDY_ABS] = {&LDY::ExecuteABS, CYC_INS_LDY_ABS, 0};
    table[INS_LDY_ABSX] = {&LDY::ExecuteABSX, CYC_INS_LDY_ABSX, 0};
    table[INS_JMP_ABS] = {&JMP::ExecuteABS, CYC_INS_JMP_ABS, 0};
    table[INS_JMP_IND] = {&JMP::ExecuteIND, CYC_INS_JMP_IND, 0};
    table[INS_JMP_ABSX] = {&JMP::ExecuteABSX, CYC_INS_JMP_ABSX, 0};
    table[INS_STA_ZP] = {&STA::ExecuteZP, CYC_INS_STA_ZP, 0};
    table[INS_STA_ZPX] = {&STA::ExecuteZPX, CYC_INS_STA_ZPX, 0};
    table[INS_STA_ABS] = {&STA::ExecuteABS, CYC_INS_STA_ABS, 0};
    table[INS_STA_ABSX] = {&STA::ExecuteABSX, CYC_INS_STA_ABSX, 0};
    table[INS_STA_ABSY] = {&STA::ExecuteABSY, CYC_INS_STA_ABSY, 0};
    table[INS_STA_INDX] = {&STA::ExecuteINDX, CYC_INS_STA_INDX, 0};
    table[INS_STA_INDY] = {&STA::ExecuteINDY, CYC_INS_STA_INDY, 0};
    table[INS_STA_IND_ZP] = {&STA::ExecuteINDZP, CYC_INS_STA_IND_ZP, 0};
    table[INS_STX_ZP] = {&STX::ExecuteZP, CYC_INS_STX_ZP, 0};
    table[INS_STX_ZPY] = {&STX::ExecuteZPY, CYC_INS_STX_ZPY, 0};
    table[INS_STX_ABS] = {&STX::ExecuteABS, CYC_INS_STX_ABS, 0};
    table[INS_STY_ZP] = {&STY::ExecuteZP, CYC_INS_STY_ZP, 0};
    table[INS_STY_ZPX] = {&STY::ExecuteZPX, CYC_INS_STY_ZPX, 0};
    table[INS_STY_ABS] = {&STY::ExecuteABS, CYC_INS_STY_ABS, 0};
    table[INS_STZ_ZP] = {&STZ::ExecuteZP, CYC_INS_STZ_ZP, 0};
    table[INS_STZ_ZPX] = {&STZ::ExecuteZPX, CYC_INS_STZ_ZPX, 0};
    table[INS_STZ_ABS] = {&STZ::ExecuteABS, CYC_INS_STZ_ABS, 0};
    table[INS_STZ_ABSX] = {&STZ::ExecuteABSX, CYC_INS_STZ_ABSX, 0};
    table[INS_TSX] = {&TSX::Execute, CYC_INS_TSX, 0};
    table[INS_TXS] = {&TXS::Execute, CYC_INS_TXS, 0};
    table[INS_PHA] = {&PHA::Execute, CYC_INS_PHA, 0};
    table[INS_PHP] = {&PHP::Execute, CYC_INS_PHP, 0};
    table[INS_PHX] = {&PHX::Execute, CYC_INS_PHX, 0};
    table[INS_PHY] = {&PHY::Execute, CYC_INS_PHY, 0};
    table[INS_PLA] = {&PLA::Execute, CYC_INS_PLA, 0};
    table[INS_PLP] = {&PLP::Execute, CYC_INS_PLP, 0};
    table[INS_PLX] = {&PLX::Execute, CYC_INS_PLX, 0};
    table[INS_PLY] = {&PLY::Execute, CYC_INS_PLY, 0};
    table[INS_TAX] = {&TAX::Execute, CYC_INS_TAX, 0};
    table[INS_TXA] = {&TXA::Execute, CYC_INS_TXA, 0};
    table[INS_TAY] = {&TAY::Execute, CYC_INS_TAY, 0};
    table[INS_TYA] = {&TYA::Execute, CYC_INS_TYA, 0};
    table[INS_AND_IM] = {&AND::ExecuteImmediate, CYC_INS_AND_IM, 0};
    table[INS_AND_ZP] = {&AND::ExecuteZP, CYC_INS_AND_ZP, 0};
    table[INS_AND_ZPX] = {&AND::ExecuteZPX, CYC_INS_AND_ZPX, 0};
    table[INS_AND_ABS] = {&AND::ExecuteABS, CYC_INS_AND_ABS, 0};
    table[INS_AND_ABSX] = {&AND::ExecuteABSX, CYC_INS_AND_ABSX, 0};
    table[INS_AND_ABSY] = {&AND::ExecuteABSY, CYC_INS_AND_ABSY, 0};
    table[INS_AND_INDX] = {&AND::ExecuteINDX, CYC_INS_AND_INDX, 0};
    table[INS_AND_INDY] = {&AND::ExecuteINDY, CYC_INS_AND_INDY, 0};
    table[INS_AND_IND_ZP] = {&AND::ExecuteIND_ZP, CYC_INS_AND_IND_ZP, 0};
    table[INS_EOR_IM] = {&EOR::ExecuteImmediate, CYC_INS_EOR_IM, 0};
    table[INS_EOR_ZP] = {&EOR::ExecuteZP, CYC_INS_EOR_ZP, 0};
    table[INS_EOR_ZPX] = {&EOR::ExecuteZPX, CYC_INS_EOR_ZPX, 0};
    table[INS_EOR_ABS] = {&EOR::ExecuteABS, CYC_INS_EOR_ABS, 0};
    table[INS_EOR_ABSX] = {&EOR::ExecuteABSX, CYC_INS_EOR_ABSX, 0};
    table[INS_EOR_ABSY] = {&EOR::ExecuteABSY, CYC_INS_EOR_ABSY, 0};
    table[INS_EOR_INDX] = {&EOR::ExecuteINDX, CYC_INS_EOR_INDX, 0};
    table[INS_EOR_INDY] = {&EOR::ExecuteINDY, CYC_INS_EOR_INDY, 0};
    table[INS_EOR_IND_ZP] = {&EOR::ExecuteIND_ZP, CYC_INS_EOR_IND_ZP, 0};
    table[INS_ORA_IM] = {&ORA::ExecuteImmediate, CYC_INS_ORA_IM, 0};
    table[INS_ORA_ZP] = {&ORA::ExecuteZP, CYC_INS_ORA_ZP, 0};
    table[INS_ORA_ZPX] = {&ORA::ExecuteZPX, CYC_INS_ORA_ZPX, 0};
    table[INS_ORA_ABS] = {&ORA::ExecuteABS, CYC_INS_ORA_ABS, 0};
    table[INS_ORA_ABSX] = {&ORA::ExecuteABSX, CYC_INS_ORA_ABSX, 0};
    table[INS_ORA_ABSY] = {&ORA::ExecuteABSY, CYC_INS_ORA_ABSY, 0};
    table[INS_ORA_INDX] = {&ORA::ExecuteINDX, CYC_INS_ORA_INDX, 0};
    table[INS_ORA_INDY] = {&ORA::ExecuteINDY, CYC_INS_ORA_INDY, 0};
    table[INS_ORA_IND_ZP] = {&ORA::ExecuteIND_ZP, CYC_INS_ORA_IND_ZP, 0};
    table[INS_BIT_IM] = {&BIT::ExecuteImmediate, CYC_INS_BIT_IM, 0};
    table[INS_BIT_ZP] = {&BIT::ExecuteZP, CYC_INS_BIT_ZP, 0};
    table[INS_BIT_ZPX] = {&BIT::ExecuteZPX, CYC_INS_BIT_ZPX, 0};
    table[INS_BIT_ABS] = {&BIT::ExecuteABS, CYC_INS_BIT_ABS, 0};
    table[INS_BIT_ABSX] = {&BIT::ExecuteABSX, CYC_INS_BIT_ABSX, 0};
    table[INS_DEX] = {&DEX::Execute, CYC_INS_DEX, 0};
    table[INS_DEY] = {&DEY::Execute, CYC_INS_DEY, 0};
    table[INS_INX] = {&INX::Execute, CYC_INS_INX, 0};
    table[INS_INY] = {&INY::Execute, CYC_INS_INY, 0};
    table[INS_INC_A] = {&INC::ExecuteAccumulator, CYC_INS_INC_A, 0};
    table[INS_INC_ZP] = {&INC::ExecuteZP, CYC_INS_INC_ZP, 0};
    table[INS_INC_ZPX] = {&INC::ExecuteZPX, CYC_INS_INC_ZPX, 0};
    table[INS_INC_ABS] = {&INC::ExecuteABS, CYC_INS_INC_ABS, 0};
    table[INS_INC_ABSX] = {&INC::ExecuteABSX, CYC_INS_INC_ABSX, 0};
    table[INS_DEC_A] = {&DEC::ExecuteAccumulator, CYC_INS_DEC_A, 0};
    table[INS_DEC_ZP] = {&DEC::ExecuteZP, CYC_INS_DEC_ZP, 0};
    table[INS_DEC_ZPX] = {&DEC::ExecuteZPX, CYC_INS_DEC_ZPX, 0};
    table[INS_DEC_ABS] = {&DEC::ExecuteABS, CYC_INS_DEC_ABS, 0};
    table[INS_DEC_ABSX] = {&DEC::ExecuteABSX, CYC_INS_DEC_ABSX, 0};
    table[INS_ADC_IM] = {&ADC::ExecuteImmediate, CYC_INS_ADC_IM, 0};
    table[INS_ADC_ZP] = {&ADC::ExecuteZP, CYC_INS_ADC_ZP, 0};
    table[INS_ADC_ZPX] = {&ADC::ExecuteZPX, CYC_INS_ADC_ZPX, 0};
    table[INS_ADC_ABS] = {&ADC::ExecuteABS, CYC_INS_ADC_ABS, 0};
    table[INS_ADC_ABSX] = {&ADC::ExecuteABSX, CYC_INS_ADC_ABSX, 0};
    table[INS_ADC_ABSY] = {&ADC::ExecuteABSY, CYC_INS_ADC_ABSY, 0};
    table[INS_ADC_INDX] = {&ADC::ExecuteINDX, CYC_INS_ADC_INDX, 0};
    table[INS_ADC_INDY] = {&ADC::ExecuteINDY, CYC_INS_ADC_INDY, 0};
    table[INS_ADC_IND_ZP] = {&ADC::ExecuteIND_ZP, CYC_INS_ADC_IND_ZP, 0};
    table[INS_SBC_IM] = {&SBC::ExecuteImmediate, CYC_INS_SBC_IM, 0};
    table[INS_SBC_ZP] = {&SBC::ExecuteZP, CYC_INS_SBC_ZP, 0};
    table[INS_SBC_ZPX] = {&SBC::ExecuteZPX, CYC_INS_SBC_ZPX, 0};
    table[INS_SBC_ABS] = {&SBC::ExecuteABS, CYC_INS_SBC_ABS, 0};
    table[INS_SBC_ABSX] = {&SBC::ExecuteABSX, CYC_INS_SBC_ABSX, 0};
    table[INS_SBC_ABSY] = {&SBC::ExecuteABSY, CYC_INS_SBC_ABSY, 0};
    table[INS_SBC_INDX] = {&SBC::ExecuteINDX, CYC_INS_SBC_INDX, 0};
    table[INS_SBC_INDY] = {&SBC::ExecuteINDY, CYC_INS_SBC_INDY, 0};
    table[INS_SBC_IND_ZP] = {&SBC::ExecuteIND_ZP, CYC_INS_SBC_IND_ZP, 0};
    table[INS_CMP_IM] = {&CMP::ExecuteImmediate, CYC_INS_CMP_IM, 0};
    table[INS_CMP_ZP] = {&CMP::ExecuteZP, CYC_INS_CMP_ZP, 0};
    table[INS_CMP_ZPX] = {&CMP::ExecuteZPX, CYC_INS_CMP_ZPX, 0};
    table[INS_CMP_ABS] = {&CMP::ExecuteABS, CYC_INS_CMP_ABS, 0};
    table[INS_CMP_ABSX] = {&CMP::ExecuteABSX, CYC_INS_CMP_ABSX, 0};
    table[INS_CMP_ABSY] = {&CMP::ExecuteABSY, CYC_INS_CMP_ABSY, 0};
    table[INS_CMP_INDX] = {&CMP::ExecuteINDX, CYC_INS_CMP_INDX, 0};
    table[INS_CMP_INDY] = {&CMP::ExecuteINDY, CYC_INS_CMP_INDY, 0};
    table[INS_CMP_IND_ZP] = {&CMP::ExecuteIND_ZP, CYC_INS_CMP_IND_ZP, 0};
    table[INS_CPX_IM] = {&CPX::ExecuteImmediate, CYC_INS_CPX_IM, 0};
    table[INS_CPX_ZP] = {&CPX::ExecuteZP, CYC_INS_CPX_ZP, 0};
    table[INS_CPX_ABS] = {&CPX::ExecuteABS, CYC_INS_CPX_ABS, 0};
    table[INS_CPY_IM] = {&CPY::ExecuteImmediate, CYC_INS_CPY_IM, 0};
    table[INS_CPY_ZP] = {&CPY::ExecuteZP, CYC_INS_CPY_ZP, 0};
    table[INS_CPY_ABS] = {&CPY::ExecuteABS, CYC_INS_CPY_ABS, 0};
    table[INS_ASL_A] = {&ASL::ExecuteAccumulator, CYC_INS_ASL_A, 0};
    table[INS_ASL_ZP] = {&ASL::ExecuteZP, CYC_INS_ASL_ZP, 0};
    table[INS_ASL_ZPX] = {&ASL::ExecuteZPX, CYC_INS_ASL_ZPX, 0};
    table[INS_ASL_ABS] = {&ASL::ExecuteABS, CYC_INS_ASL_ABS, 0};
    table[INS_ASL_ABSX] = {&ASL::ExecuteABSX, CYC_INS_ASL_ABSX, 0};
    table[INS_LSR_A] = {&LSR::ExecuteAccumulator, CYC_INS_LSR_A, 0};
    table[INS_LSR_ZP] = {&LSR::ExecuteZP, CYC_INS_LSR_ZP, 0};
    table[INS_LSR_ZPX] = {&LSR::ExecuteZPX, CYC_INS_LSR_ZPX, 0};
    table[INS_LSR_ABS] = {&LSR::ExecuteABS, CYC_INS_LSR_ABS, 0};
    table[INS_LSR_ABSX] = {&LSR::ExecuteABSX, CYC_INS_LSR_ABSX, 0};
    table[INS_ROL_A] = {&ROL::ExecuteAccumulator, CYC_INS_ROL_A, 0};
    table[INS_ROL_ZP] = {&ROL::ExecuteZP, CYC_INS_ROL_ZP, 0};
    table[INS_ROL_ZPX] = {&ROL::ExecuteZPX, CYC_INS_ROL_ZPX, 0};
    table[INS_ROL_ABS] = {&ROL::ExecuteABS, CYC_INS_ROL_ABS, 0};
    table[INS_ROL_ABSX] = {&ROL::ExecuteABSX, CYC_INS_ROL_ABSX, 0};
    table[INS_ROR_A] = {&ROR::ExecuteAccumulator, CYC_INS_ROR_A, 0};
    table[INS_ROR_ZP] = {&ROR::ExecuteZP, CYC_INS_ROR_ZP, 0};
    table[INS_ROR_ZPX] = {&ROR::ExecuteZPX, CYC_INS_ROR_ZPX, 0};
    table[INS_ROR_ABS] = {&ROR::ExecuteABS, CYC_INS_ROR_ABS, 0};
    table[INS_ROR_ABSX] = {&ROR::ExecuteABSX, CYC_INS_ROR_ABSX, 0};
    table[INS_BCC] = {&BCC::Execute, CYC_INS_BCC, 0};
    table[INS_BRK] = {&BRK::Execute, CYC_INS_BRK, 0};
    table[INS_BCS] = {&BCS::Execute, CYC_INS_BCS, 0};
    table[INS_BNE] = {&BNE::Execute, CYC_INS_BNE, 0};
    table[INS_BEQ] = {&BEQ::Execute, CYC_INS_BEQ, 0};
    table[INS_BMI] = {&BMI::Execute, CYC_INS_BMI, 0};
    table[INS_BPL] = {&BPL::Execute, CYC_INS_BPL, 0};
    table[INS_BVS] = {&BVS::Execute, CYC_INS_BVS, 0};
    table[INS_BVC] = {&BVC::Execute, CYC_INS_BVC, 0};
    table[INS_CLC] = {&CLC::Execute, CYC_INS_CLC, 0};
    table[INS_CLI] = {&CLI::Execute, CYC_INS_CLI, 0};
    table[INS_CLD] = {&CLD::Execute, CYC_INS_CLD, 0};
    table[INS_SEC] = {&SEC::Execute, CYC_INS_SEC, 0};
    table[INS_SEI] = {&SEI::Execute, CYC_INS_SEI, 0};
    table[INS_SED] = {&SED::Execute, CYC_INS_SED, 0};
    table[INS_CLV] = {&CLV::Execute, CYC_INS_CLV, 0};
    table[INS_RTI] = {&RTI::Execute, CYC_INS_RTI, 0};
    table[INS_RMB0] = {&RMB::Execute0, CYC_INS_RMB0, 0};
    table[INS_RMB1] = {&RMB::Execute1, CYC_INS_RMB1, 0};
    table[INS_RMB2] = {&RMB::Execute2, CYC_INS_RMB2, 0};
    table[INS_RMB3] = {&RMB::Execute3, CYC_INS_RMB3, 0};
    table[INS_RMB4] = {&RMB::Execute4, CYC_INS_RMB4, 0};
    table[INS_RMB5] = {&RMB::Execute5, CYC_INS_RMB5, 0};
    table[INS_RMB6] = {&RMB::Execute6, CYC_INS_RMB6, 0};
    table[INS_RMB7] = {&RMB::Execute7, CYC_INS_RMB7, 0};
    table[INS_SMB0] = {&SMB::Execute0, CYC_INS_SMB0, 0};
    table[INS_SMB1] = {&SMB::Execute1, CYC_INS_SMB1, 0};
    table[INS_SMB2] = {&SMB::Execute2, CYC_INS_SMB2, 0};
    table[INS_SMB3] = {&SMB::Execute3, CYC_INS_SMB3, 0};
    table[INS_SMB4] = {&SMB::Execute4, CYC_INS_SMB4, 0};
    table[INS_SMB5] = {&SMB::Execute5, CYC_INS_SMB5, 0};
    table[INS_SMB6] = {&SMB::Execute6, CYC_INS_SMB6, 0};
    table[INS_SMB7] = {&SMB::Execute7, CYC_INS_SMB7, 0};
    table[INS_BBR0] = {&BBR::Execute0, CYC_INS_BBR0, 0};
    table[INS_BBR1] = {&BBR::Execute1, CYC_INS_BBR1, 0};
    table[INS_BBR2] = {&BBR::Execute2, CYC_INS_BBR2, 0};
    table[INS_BBR3] = {&BBR::Execute3, CYC_INS_BBR3, 0};
    table[INS_BBR4] = {&BBR::Execute4, CYC_INS_BBR4, 0};
    table[INS_BBR5] = {&BBR::Execute5, CYC_INS_BBR5, 0};
    table[INS_BBR6] = {&BBR::Execute6, CYC_INS_BBR6, 0};
    table[INS_BBR7] = {&BBR::Execute7, CYC_INS_BBR7, 0};
    table[INS_BBS0] = {&BBS::Execute0, CYC_INS_BBS0, 0};
    table[INS_BBS1] = {&BBS::Execute1, CYC_INS_BBS1, 0};
    table[INS_BBS2] = {&BBS::Execute2, CYC_INS_BBS2, 0};
    table[INS_BBS3] = {&BBS::Execute3, CYC_INS_BBS3, 0};
    table[INS_BBS4] = {&BBS::Execute4, CYC_INS_BBS4, 0};
    table[INS_BBS5] = {&BBS::Execute5, CYC_INS_BBS5, 0};
    table[INS_BBS6] = {&BBS::Execute6, CYC_INS_BBS6, 0};
    table[INS_BBS7] = {&BBS::Execute7, CYC_INS_BBS7, 0};
    table[INS_TRB_ZP] = {&TRB::ExecuteZP, CYC_INS_TRB_ZP, 0};
    table[INS_TRB_ABS] = {&TRB::ExecuteABS, CYC_INS_TRB_ABS, 0};
    table[INS_TSB_ZP] = {&TSB::ExecuteZP, CYC_INS_TSB_ZP, 0};
    table[INS_TSB_ABS] = {&TSB::ExecuteABS, CYC_INS_TSB_ABS, 0};

    return table;
}

constexpr std::array<OpcodeEntry, 256> dispatchTable = BuildDispatchTable();

} // namespace CPUDispatch
} // namespace Hardware
