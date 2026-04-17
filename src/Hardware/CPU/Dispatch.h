#pragma once
#include <cstdint>

#include "Hardware/Core/Bus.h"

namespace Hardware {

class CPU;

namespace CPUDispatch {

template <bool Debug>
using InstructionExecutor = void (*)(CPU&, Bus&);

template <bool Debug>
struct OpcodeEntry {
    InstructionExecutor<Debug> executor;
    uint8_t baseCycles;
    int8_t exitCode;
};

template <bool Debug>
inline int Dispatch(CPU& cpu, Bus& bus);

}  // namespace CPUDispatch

}  // namespace Hardware

#include <array>

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

namespace Hardware::CPUDispatch {

template <bool Debug>
constexpr std::array<OpcodeEntry<Debug>, 256> BuildDispatchTable() {
    std::array<OpcodeEntry<Debug>, 256> table{};
    for (auto& entry : table) {
        entry = {nullptr, 0, -1};
    }

    table[INS_WAI] = {[](CPU& cpu, Bus&) { cpu.waiting = true; }, 0, 0};
    table[INS_STP] = {nullptr, 0, 1};
    table[INS_NOP_IM_02] = {&NOP::ExecuteImmediate<Debug>, CYC_INS_NOP_IM, 0};
    table[INS_BRA] = {&BRA::Execute<Debug>, CYC_INS_BRA, 0};
    table[INS_NOP] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[INS_LDA_IM] = {&LDA::ExecuteImmediate<Debug>, CYC_INS_LDA_IM, 0};
    table[INS_LDA_ZP] = {&LDA::ExecuteZP<Debug>, CYC_INS_LDA_ZP, 0};
    table[INS_LDA_ZPX] = {&LDA::ExecuteZPX<Debug>, CYC_INS_LDA_ZPX, 0};
    table[INS_LDA_ABS] = {&LDA::ExecuteABS<Debug>, CYC_INS_LDA_ABS, 0};
    table[INS_LDA_ABSX] = {&LDA::ExecuteABSX<Debug>, CYC_INS_LDA_ABSX, 0};
    table[INS_LDA_ABSY] = {&LDA::ExecuteABSY<Debug>, CYC_INS_LDA_ABSY, 0};
    table[INS_LDA_INDX] = {&LDA::ExecuteINDX<Debug>, CYC_INS_LDA_INDX, 0};
    table[INS_LDA_INDY] = {&LDA::ExecuteINDY<Debug>, CYC_INS_LDA_INDY, 0};
    table[INS_LDA_IND_ZP] = {&LDA::ExecuteIND_ZP<Debug>, CYC_INS_LDA_IND_ZP, 0};
    table[INS_JSR] = {&JSR::Execute<Debug>, CYC_INS_JSR, 0};
    table[INS_RTS] = {&RTS::Execute<Debug>, CYC_INS_RTS, 0};
    table[INS_LDX_IM] = {&LDX::ExecuteImmediate<Debug>, CYC_INS_LDX_IM, 0};
    table[INS_LDX_ZP] = {&LDX::ExecuteZP<Debug>, CYC_INS_LDX_ZP, 0};
    table[INS_LDX_ZPY] = {&LDX::ExecuteZPY<Debug>, CYC_INS_LDX_ZPY, 0};
    table[INS_LDX_ABS] = {&LDX::ExecuteABS<Debug>, CYC_INS_LDX_ABS, 0};
    table[INS_LDX_ABSY] = {&LDX::ExecuteABSY<Debug>, CYC_INS_LDX_ABSY, 0};
    table[INS_LDY_IM] = {&LDY::ExecuteImmediate<Debug>, CYC_INS_LDY_IM, 0};
    table[INS_LDY_ZP] = {&LDY::ExecuteZP<Debug>, CYC_INS_LDY_ZP, 0};
    table[INS_LDY_ZPX] = {&LDY::ExecuteZPX<Debug>, CYC_INS_LDY_ZPX, 0};
    table[INS_LDY_ABS] = {&LDY::ExecuteABS<Debug>, CYC_INS_LDY_ABS, 0};
    table[INS_LDY_ABSX] = {&LDY::ExecuteABSX<Debug>, CYC_INS_LDY_ABSX, 0};
    table[INS_JMP_ABS] = {&JMP::ExecuteABS<Debug>, CYC_INS_JMP_ABS, 0};
    table[INS_JMP_IND] = {&JMP::ExecuteIND<Debug>, CYC_INS_JMP_IND, 0};
    table[INS_JMP_ABSX] = {&JMP::ExecuteABSX<Debug>, CYC_INS_JMP_ABSX, 0};
    table[INS_STA_ZP] = {&STA::ExecuteZP<Debug>, CYC_INS_STA_ZP, 0};
    table[INS_STA_ZPX] = {&STA::ExecuteZPX<Debug>, CYC_INS_STA_ZPX, 0};
    table[INS_STA_ABS] = {&STA::ExecuteABS<Debug>, CYC_INS_STA_ABS, 0};
    table[INS_STA_ABSX] = {&STA::ExecuteABSX<Debug>, CYC_INS_STA_ABSX, 0};
    table[INS_STA_ABSY] = {&STA::ExecuteABSY<Debug>, CYC_INS_STA_ABSY, 0};
    table[INS_STA_INDX] = {&STA::ExecuteINDX<Debug>, CYC_INS_STA_INDX, 0};
    table[INS_STA_INDY] = {&STA::ExecuteINDY<Debug>, CYC_INS_STA_INDY, 0};
    table[INS_STA_IND_ZP] = {&STA::ExecuteINDZP<Debug>, CYC_INS_STA_IND_ZP, 0};
    table[INS_STX_ZP] = {&STX::ExecuteZP<Debug>, CYC_INS_STX_ZP, 0};
    table[INS_STX_ZPY] = {&STX::ExecuteZPY<Debug>, CYC_INS_STX_ZPY, 0};
    table[INS_STX_ABS] = {&STX::ExecuteABS<Debug>, CYC_INS_STX_ABS, 0};
    table[INS_STY_ZP] = {&STY::ExecuteZP<Debug>, CYC_INS_STY_ZP, 0};
    table[INS_STY_ZPX] = {&STY::ExecuteZPX<Debug>, CYC_INS_STY_ZPX, 0};
    table[INS_STY_ABS] = {&STY::ExecuteABS<Debug>, CYC_INS_STY_ABS, 0};
    table[INS_STZ_ZP] = {&STZ::ExecuteZP<Debug>, CYC_INS_STZ_ZP, 0};
    table[INS_STZ_ZPX] = {&STZ::ExecuteZPX<Debug>, CYC_INS_STZ_ZPX, 0};
    table[INS_STZ_ABS] = {&STZ::ExecuteABS<Debug>, CYC_INS_STZ_ABS, 0};
    table[INS_STZ_ABSX] = {&STZ::ExecuteABSX<Debug>, CYC_INS_STZ_ABSX, 0};
    table[INS_TSX] = {&TSX::Execute<Debug>, CYC_INS_TSX, 0};
    table[INS_TXS] = {&TXS::Execute<Debug>, CYC_INS_TXS, 0};
    table[INS_PHA] = {&PHA::Execute<Debug>, CYC_INS_PHA, 0};
    table[INS_PHP] = {&PHP::Execute<Debug>, CYC_INS_PHP, 0};
    table[INS_PHX] = {&PHX::Execute<Debug>, CYC_INS_PHX, 0};
    table[INS_PHY] = {&PHY::Execute<Debug>, CYC_INS_PHY, 0};
    table[INS_PLA] = {&PLA::Execute<Debug>, CYC_INS_PLA, 0};
    table[INS_PLP] = {&PLP::Execute<Debug>, CYC_INS_PLP, 0};
    table[INS_PLX] = {&PLX::Execute<Debug>, CYC_INS_PLX, 0};
    table[INS_PLY] = {&PLY::Execute<Debug>, CYC_INS_PLY, 0};
    table[INS_TAX] = {&TAX::Execute<Debug>, CYC_INS_TAX, 0};
    table[INS_TXA] = {&TXA::Execute<Debug>, CYC_INS_TXA, 0};
    table[INS_TAY] = {&TAY::Execute<Debug>, CYC_INS_TAY, 0};
    table[INS_TYA] = {&TYA::Execute<Debug>, CYC_INS_TYA, 0};
    table[INS_AND_IM] = {&AND::ExecuteImmediate<Debug>, CYC_INS_AND_IM, 0};
    table[INS_AND_ZP] = {&AND::ExecuteZP<Debug>, CYC_INS_AND_ZP, 0};
    table[INS_AND_ZPX] = {&AND::ExecuteZPX<Debug>, CYC_INS_AND_ZPX, 0};
    table[INS_AND_ABS] = {&AND::ExecuteABS<Debug>, CYC_INS_AND_ABS, 0};
    table[INS_AND_ABSX] = {&AND::ExecuteABSX<Debug>, CYC_INS_AND_ABSX, 0};
    table[INS_AND_ABSY] = {&AND::ExecuteABSY<Debug>, CYC_INS_AND_ABSY, 0};
    table[INS_AND_INDX] = {&AND::ExecuteINDX<Debug>, CYC_INS_AND_INDX, 0};
    table[INS_AND_INDY] = {&AND::ExecuteINDY<Debug>, CYC_INS_AND_INDY, 0};
    table[INS_AND_IND_ZP] = {&AND::ExecuteIND_ZP<Debug>, CYC_INS_AND_IND_ZP, 0};
    table[INS_EOR_IM] = {&EOR::ExecuteImmediate<Debug>, CYC_INS_EOR_IM, 0};
    table[INS_EOR_ZP] = {&EOR::ExecuteZP<Debug>, CYC_INS_EOR_ZP, 0};
    table[INS_EOR_ZPX] = {&EOR::ExecuteZPX<Debug>, CYC_INS_EOR_ZPX, 0};
    table[INS_EOR_ABS] = {&EOR::ExecuteABS<Debug>, CYC_INS_EOR_ABS, 0};
    table[INS_EOR_ABSX] = {&EOR::ExecuteABSX<Debug>, CYC_INS_EOR_ABSX, 0};
    table[INS_EOR_ABSY] = {&EOR::ExecuteABSY<Debug>, CYC_INS_EOR_ABSY, 0};
    table[INS_EOR_INDX] = {&EOR::ExecuteINDX<Debug>, CYC_INS_EOR_INDX, 0};
    table[INS_EOR_INDY] = {&EOR::ExecuteINDY<Debug>, CYC_INS_EOR_INDY, 0};
    table[INS_EOR_IND_ZP] = {&EOR::ExecuteIND_ZP<Debug>, CYC_INS_EOR_IND_ZP, 0};
    table[INS_ORA_IM] = {&ORA::ExecuteImmediate<Debug>, CYC_INS_ORA_IM, 0};
    table[INS_ORA_ZP] = {&ORA::ExecuteZP<Debug>, CYC_INS_ORA_ZP, 0};
    table[INS_ORA_ZPX] = {&ORA::ExecuteZPX<Debug>, CYC_INS_ORA_ZPX, 0};
    table[INS_ORA_ABS] = {&ORA::ExecuteABS<Debug>, CYC_INS_ORA_ABS, 0};
    table[INS_ORA_ABSX] = {&ORA::ExecuteABSX<Debug>, CYC_INS_ORA_ABSX, 0};
    table[INS_ORA_ABSY] = {&ORA::ExecuteABSY<Debug>, CYC_INS_ORA_ABSY, 0};
    table[INS_ORA_INDX] = {&ORA::ExecuteINDX<Debug>, CYC_INS_ORA_INDX, 0};
    table[INS_ORA_INDY] = {&ORA::ExecuteINDY<Debug>, CYC_INS_ORA_INDY, 0};
    table[INS_ORA_IND_ZP] = {&ORA::ExecuteIND_ZP<Debug>, CYC_INS_ORA_IND_ZP, 0};
    table[INS_BIT_IM] = {&BIT::ExecuteImmediate<Debug>, CYC_INS_BIT_IM, 0};
    table[INS_BIT_ZP] = {&BIT::ExecuteZP<Debug>, CYC_INS_BIT_ZP, 0};
    table[INS_BIT_ZPX] = {&BIT::ExecuteZPX<Debug>, CYC_INS_BIT_ZPX, 0};
    table[INS_BIT_ABS] = {&BIT::ExecuteABS<Debug>, CYC_INS_BIT_ABS, 0};
    table[INS_BIT_ABSX] = {&BIT::ExecuteABSX<Debug>, CYC_INS_BIT_ABSX, 0};
    table[INS_DEX] = {&DEX::Execute<Debug>, CYC_INS_DEX, 0};
    table[INS_DEY] = {&DEY::Execute<Debug>, CYC_INS_DEY, 0};
    table[INS_INX] = {&INX::Execute<Debug>, CYC_INS_INX, 0};
    table[INS_INY] = {&INY::Execute<Debug>, CYC_INS_INY, 0};
    table[INS_INC_A] = {&INC::ExecuteAccumulator<Debug>, CYC_INS_INC_A, 0};
    table[INS_INC_ZP] = {&INC::ExecuteZP<Debug>, CYC_INS_INC_ZP, 0};
    table[INS_INC_ZPX] = {&INC::ExecuteZPX<Debug>, CYC_INS_INC_ZPX, 0};
    table[INS_INC_ABS] = {&INC::ExecuteABS<Debug>, CYC_INS_INC_ABS, 0};
    table[INS_INC_ABSX] = {&INC::ExecuteABSX<Debug>, CYC_INS_INC_ABSX, 0};
    table[INS_DEC_A] = {&DEC::ExecuteAccumulator<Debug>, CYC_INS_DEC_A, 0};
    table[INS_DEC_ZP] = {&DEC::ExecuteZP<Debug>, CYC_INS_DEC_ZP, 0};
    table[INS_DEC_ZPX] = {&DEC::ExecuteZPX<Debug>, CYC_INS_DEC_ZPX, 0};
    table[INS_DEC_ABS] = {&DEC::ExecuteABS<Debug>, CYC_INS_DEC_ABS, 0};
    table[INS_DEC_ABSX] = {&DEC::ExecuteABSX<Debug>, CYC_INS_DEC_ABSX, 0};
    table[INS_ADC_IM] = {&ADC::ExecuteImmediate<Debug>, CYC_INS_ADC_IM, 0};
    table[INS_ADC_ZP] = {&ADC::ExecuteZP<Debug>, CYC_INS_ADC_ZP, 0};
    table[INS_ADC_ZPX] = {&ADC::ExecuteZPX<Debug>, CYC_INS_ADC_ZPX, 0};
    table[INS_ADC_ABS] = {&ADC::ExecuteABS<Debug>, CYC_INS_ADC_ABS, 0};
    table[INS_ADC_ABSX] = {&ADC::ExecuteABSX<Debug>, CYC_INS_ADC_ABSX, 0};
    table[INS_ADC_ABSY] = {&ADC::ExecuteABSY<Debug>, CYC_INS_ADC_ABSY, 0};
    table[INS_ADC_INDX] = {&ADC::ExecuteINDX<Debug>, CYC_INS_ADC_INDX, 0};
    table[INS_ADC_INDY] = {&ADC::ExecuteINDY<Debug>, CYC_INS_ADC_INDY, 0};
    table[INS_ADC_IND_ZP] = {&ADC::ExecuteIND_ZP<Debug>, CYC_INS_ADC_IND_ZP, 0};
    table[INS_SBC_IM] = {&SBC::ExecuteImmediate<Debug>, CYC_INS_SBC_IM, 0};
    table[INS_SBC_ZP] = {&SBC::ExecuteZP<Debug>, CYC_INS_SBC_ZP, 0};
    table[INS_SBC_ZPX] = {&SBC::ExecuteZPX<Debug>, CYC_INS_SBC_ZPX, 0};
    table[INS_SBC_ABS] = {&SBC::ExecuteABS<Debug>, CYC_INS_SBC_ABS, 0};
    table[INS_SBC_ABSX] = {&SBC::ExecuteABSX<Debug>, CYC_INS_SBC_ABSX, 0};
    table[INS_SBC_ABSY] = {&SBC::ExecuteABSY<Debug>, CYC_INS_SBC_ABSY, 0};
    table[INS_SBC_INDX] = {&SBC::ExecuteINDX<Debug>, CYC_INS_SBC_INDX, 0};
    table[INS_SBC_INDY] = {&SBC::ExecuteINDY<Debug>, CYC_INS_SBC_INDY, 0};
    table[INS_SBC_IND_ZP] = {&SBC::ExecuteIND_ZP<Debug>, CYC_INS_SBC_IND_ZP, 0};
    table[INS_CMP_IM] = {&CMP::ExecuteImmediate<Debug>, CYC_INS_CMP_IM, 0};
    table[INS_CMP_ZP] = {&CMP::ExecuteZP<Debug>, CYC_INS_CMP_ZP, 0};
    table[INS_CMP_ZPX] = {&CMP::ExecuteZPX<Debug>, CYC_INS_CMP_ZPX, 0};
    table[INS_CMP_ABS] = {&CMP::ExecuteABS<Debug>, CYC_INS_CMP_ABS, 0};
    table[INS_CMP_ABSX] = {&CMP::ExecuteABSX<Debug>, CYC_INS_CMP_ABSX, 0};
    table[INS_CMP_ABSY] = {&CMP::ExecuteABSY<Debug>, CYC_INS_CMP_ABSY, 0};
    table[INS_CMP_INDX] = {&CMP::ExecuteINDX<Debug>, CYC_INS_CMP_INDX, 0};
    table[INS_CMP_INDY] = {&CMP::ExecuteINDY<Debug>, CYC_INS_CMP_INDY, 0};
    table[INS_CMP_IND_ZP] = {&CMP::ExecuteIND_ZP<Debug>, CYC_INS_CMP_IND_ZP, 0};
    table[INS_CPX_IM] = {&CPX::ExecuteImmediate<Debug>, CYC_INS_CPX_IM, 0};
    table[INS_CPX_ZP] = {&CPX::ExecuteZP<Debug>, CYC_INS_CPX_ZP, 0};
    table[INS_CPX_ABS] = {&CPX::ExecuteABS<Debug>, CYC_INS_CPX_ABS, 0};
    table[INS_CPY_IM] = {&CPY::ExecuteImmediate<Debug>, CYC_INS_CPY_IM, 0};
    table[INS_CPY_ZP] = {&CPY::ExecuteZP<Debug>, CYC_INS_CPY_ZP, 0};
    table[INS_CPY_ABS] = {&CPY::ExecuteABS<Debug>, CYC_INS_CPY_ABS, 0};
    table[INS_ASL_A] = {&ASL::ExecuteAccumulator<Debug>, CYC_INS_ASL_A, 0};
    table[INS_ASL_ZP] = {&ASL::ExecuteZP<Debug>, CYC_INS_ASL_ZP, 0};
    table[INS_ASL_ZPX] = {&ASL::ExecuteZPX<Debug>, CYC_INS_ASL_ZPX, 0};
    table[INS_ASL_ABS] = {&ASL::ExecuteABS<Debug>, CYC_INS_ASL_ABS, 0};
    table[INS_ASL_ABSX] = {&ASL::ExecuteABSX<Debug>, CYC_INS_ASL_ABSX, 0};
    table[INS_LSR_A] = {&LSR::ExecuteAccumulator<Debug>, CYC_INS_LSR_A, 0};
    table[INS_LSR_ZP] = {&LSR::ExecuteZP<Debug>, CYC_INS_LSR_ZP, 0};
    table[INS_LSR_ZPX] = {&LSR::ExecuteZPX<Debug>, CYC_INS_LSR_ZPX, 0};
    table[INS_LSR_ABS] = {&LSR::ExecuteABS<Debug>, CYC_INS_LSR_ABS, 0};
    table[INS_LSR_ABSX] = {&LSR::ExecuteABSX<Debug>, CYC_INS_LSR_ABSX, 0};
    table[INS_ROL_A] = {&ROL::ExecuteAccumulator<Debug>, CYC_INS_ROL_A, 0};
    table[INS_ROL_ZP] = {&ROL::ExecuteZP<Debug>, CYC_INS_ROL_ZP, 0};
    table[INS_ROL_ZPX] = {&ROL::ExecuteZPX<Debug>, CYC_INS_ROL_ZPX, 0};
    table[INS_ROL_ABS] = {&ROL::ExecuteABS<Debug>, CYC_INS_ROL_ABS, 0};
    table[INS_ROL_ABSX] = {&ROL::ExecuteABSX<Debug>, CYC_INS_ROL_ABSX, 0};
    table[INS_ROR_A] = {&ROR::ExecuteAccumulator<Debug>, CYC_INS_ROR_A, 0};
    table[INS_ROR_ZP] = {&ROR::ExecuteZP<Debug>, CYC_INS_ROR_ZP, 0};
    table[INS_ROR_ZPX] = {&ROR::ExecuteZPX<Debug>, CYC_INS_ROR_ZPX, 0};
    table[INS_ROR_ABS] = {&ROR::ExecuteABS<Debug>, CYC_INS_ROR_ABS, 0};
    table[INS_ROR_ABSX] = {&ROR::ExecuteABSX<Debug>, CYC_INS_ROR_ABSX, 0};
    table[INS_BCC] = {&BCC::Execute<Debug>, CYC_INS_BCC, 0};
    table[INS_BRK] = {&BRK::Execute<Debug>, CYC_INS_BRK, 0};
    table[INS_BCS] = {&BCS::Execute<Debug>, CYC_INS_BCS, 0};
    table[INS_BNE] = {&BNE::Execute<Debug>, CYC_INS_BNE, 0};
    table[INS_BEQ] = {&BEQ::Execute<Debug>, CYC_INS_BEQ, 0};
    table[INS_BMI] = {&BMI::Execute<Debug>, CYC_INS_BMI, 0};
    table[INS_BPL] = {&BPL::Execute<Debug>, CYC_INS_BPL, 0};
    table[INS_BVS] = {&BVS::Execute<Debug>, CYC_INS_BVS, 0};
    table[INS_BVC] = {&BVC::Execute<Debug>, CYC_INS_BVC, 0};
    table[INS_CLC] = {&CLC::Execute<Debug>, CYC_INS_CLC, 0};
    table[INS_CLI] = {&CLI::Execute<Debug>, CYC_INS_CLI, 0};
    table[INS_CLD] = {&CLD::Execute<Debug>, CYC_INS_CLD, 0};
    table[INS_SEC] = {&SEC::Execute<Debug>, CYC_INS_SEC, 0};
    table[INS_SEI] = {&SEI::Execute<Debug>, CYC_INS_SEI, 0};
    table[INS_SED] = {&SED::Execute<Debug>, CYC_INS_SED, 0};
    table[INS_CLV] = {&CLV::Execute<Debug>, CYC_INS_CLV, 0};
    table[INS_RTI] = {&RTI::Execute<Debug>, CYC_INS_RTI, 0};
    table[INS_RMB0] = {&RMB::Execute0<Debug>, CYC_INS_RMB0, 0};
    table[INS_RMB1] = {&RMB::Execute1<Debug>, CYC_INS_RMB1, 0};
    table[INS_RMB2] = {&RMB::Execute2<Debug>, CYC_INS_RMB2, 0};
    table[INS_RMB3] = {&RMB::Execute3<Debug>, CYC_INS_RMB3, 0};
    table[INS_RMB4] = {&RMB::Execute4<Debug>, CYC_INS_RMB4, 0};
    table[INS_RMB5] = {&RMB::Execute5<Debug>, CYC_INS_RMB5, 0};
    table[INS_RMB6] = {&RMB::Execute6<Debug>, CYC_INS_RMB6, 0};
    table[INS_RMB7] = {&RMB::Execute7<Debug>, CYC_INS_RMB7, 0};
    table[INS_SMB0] = {&SMB::Execute0<Debug>, CYC_INS_SMB0, 0};
    table[INS_SMB1] = {&SMB::Execute1<Debug>, CYC_INS_SMB1, 0};
    table[INS_SMB2] = {&SMB::Execute2<Debug>, CYC_INS_SMB2, 0};
    table[INS_SMB3] = {&SMB::Execute3<Debug>, CYC_INS_SMB3, 0};
    table[INS_SMB4] = {&SMB::Execute4<Debug>, CYC_INS_SMB4, 0};
    table[INS_SMB5] = {&SMB::Execute5<Debug>, CYC_INS_SMB5, 0};
    table[INS_SMB6] = {&SMB::Execute6<Debug>, CYC_INS_SMB6, 0};
    table[INS_SMB7] = {&SMB::Execute7<Debug>, CYC_INS_SMB7, 0};
    table[INS_BBR0] = {&BBR::Execute0<Debug>, CYC_INS_BBR0, 0};
    table[INS_BBR1] = {&BBR::Execute1<Debug>, CYC_INS_BBR1, 0};
    table[INS_BBR2] = {&BBR::Execute2<Debug>, CYC_INS_BBR2, 0};
    table[INS_BBR3] = {&BBR::Execute3<Debug>, CYC_INS_BBR3, 0};
    table[INS_BBR4] = {&BBR::Execute4<Debug>, CYC_INS_BBR4, 0};
    table[INS_BBR5] = {&BBR::Execute5<Debug>, CYC_INS_BBR5, 0};
    table[INS_BBR6] = {&BBR::Execute6<Debug>, CYC_INS_BBR6, 0};
    table[INS_BBR7] = {&BBR::Execute7<Debug>, CYC_INS_BBR7, 0};
    table[INS_BBS0] = {&BBS::Execute0<Debug>, CYC_INS_BBS0, 0};
    table[INS_BBS1] = {&BBS::Execute1<Debug>, CYC_INS_BBS1, 0};
    table[INS_BBS2] = {&BBS::Execute2<Debug>, CYC_INS_BBS2, 0};
    table[INS_BBS3] = {&BBS::Execute3<Debug>, CYC_INS_BBS3, 0};
    table[INS_BBS4] = {&BBS::Execute4<Debug>, CYC_INS_BBS4, 0};
    table[INS_BBS5] = {&BBS::Execute5<Debug>, CYC_INS_BBS5, 0};
    table[INS_BBS6] = {&BBS::Execute6<Debug>, CYC_INS_BBS6, 0};
    table[INS_BBS7] = {&BBS::Execute7<Debug>, CYC_INS_BBS7, 0};
    table[INS_TRB_ZP] = {&TRB::ExecuteZP<Debug>, CYC_INS_TRB_ZP, 0};
    table[INS_TRB_ABS] = {&TRB::ExecuteABS<Debug>, CYC_INS_TRB_ABS, 0};
    table[INS_TSB_ZP] = {&TSB::ExecuteZP<Debug>, CYC_INS_TSB_ZP, 0};
    table[INS_TSB_ABS] = {&TSB::ExecuteABS<Debug>, CYC_INS_TSB_ABS, 0};

    table[0x03] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x0B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x13] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x1B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x22] = {&NOP::ExecuteImmediate<Debug>, CYC_INS_NOP_IM, 0};
    table[0x23] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x2B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x33] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x3B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x42] = {&NOP::ExecuteImmediate<Debug>, CYC_INS_NOP_IM, 0};
    table[0x43] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x44] = {&NOP::ExecuteZP<Debug>, CYC_INS_NOP_ZP, 0};
    table[0x4B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x53] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x54] = {&NOP::ExecuteZPX<Debug>, CYC_INS_NOP_ZPX, 0};
    table[0x5B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x5C] = {&NOP::ExecuteABS<Debug>, CYC_INS_NOP_ABS, 0};
    table[0x62] = {&NOP::ExecuteImmediate<Debug>, CYC_INS_NOP_IM, 0};
    table[0x63] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x6B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x73] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x7B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x82] = {&NOP::ExecuteImmediate<Debug>, CYC_INS_NOP_IM, 0};
    table[0x83] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x8B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x93] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0x9B] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xA3] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xAB] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xB3] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xBB] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xC2] = {&NOP::ExecuteImmediate<Debug>, CYC_INS_NOP_IM, 0};
    table[0xC3] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xD3] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xD4] = {&NOP::ExecuteZPX<Debug>, CYC_INS_NOP_ZPX, 0};
    table[0xDC] = {&NOP::ExecuteABSX<Debug>, CYC_INS_NOP_ABSX, 0};
    table[0xE2] = {&NOP::ExecuteImmediate<Debug>, CYC_INS_NOP_IM, 0};
    table[0xE3] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xEB] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xF3] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xF4] = {&NOP::ExecuteZPX<Debug>, CYC_INS_NOP_ZPX, 0};
    table[0xFB] = {&NOP::Execute<Debug>, CYC_INS_NOP, 0};
    table[0xFC] = {&NOP::ExecuteABSX<Debug>, CYC_INS_NOP_ABSX, 0};

    return table;
}

template <bool Debug>
constexpr std::array<OpcodeEntry<Debug>, 256> dispatchTable = BuildDispatchTable<Debug>();

}  // namespace Hardware::CPUDispatch

namespace Hardware::CPUDispatch {

template <bool Debug>
inline int Dispatch(CPU& cpu, Bus& bus) {
    Byte opcode = cpu.FetchByte<Debug>(bus);

#define HOT_OPCODE_LIST(X)                                                                                             \
    X(INS_LDA_IM, hot_lda_im, LDA::ExecuteImmediate, CYC_INS_LDA_IM)                                                  \
    X(INS_LDA_ZP, hot_lda_zp, LDA::ExecuteZP, CYC_INS_LDA_ZP)                                                         \
    X(INS_LDA_ZPX, hot_lda_zpx, LDA::ExecuteZPX, CYC_INS_LDA_ZPX)                                                     \
    X(INS_LDA_ABS, hot_lda_abs, LDA::ExecuteABS, CYC_INS_LDA_ABS)                                                     \
    X(INS_LDA_ABSX, hot_lda_absx, LDA::ExecuteABSX, CYC_INS_LDA_ABSX)                                                 \
    X(INS_LDA_ABSY, hot_lda_absy, LDA::ExecuteABSY, CYC_INS_LDA_ABSY)                                                 \
    X(INS_LDA_INDX, hot_lda_indx, LDA::ExecuteINDX, CYC_INS_LDA_INDX)                                                 \
    X(INS_LDA_INDY, hot_lda_indy, LDA::ExecuteINDY, CYC_INS_LDA_INDY)                                                 \
    X(INS_LDA_IND_ZP, hot_lda_ind_zp, LDA::ExecuteIND_ZP, CYC_INS_LDA_IND_ZP)                                        \
    X(INS_STA_ZP, hot_sta_zp, STA::ExecuteZP, CYC_INS_STA_ZP)                                                         \
    X(INS_STA_ZPX, hot_sta_zpx, STA::ExecuteZPX, CYC_INS_STA_ZPX)                                                     \
    X(INS_STA_ABS, hot_sta_abs, STA::ExecuteABS, CYC_INS_STA_ABS)                                                     \
    X(INS_STA_ABSX, hot_sta_absx, STA::ExecuteABSX, CYC_INS_STA_ABSX)                                                 \
    X(INS_STA_ABSY, hot_sta_absy, STA::ExecuteABSY, CYC_INS_STA_ABSY)                                                 \
    X(INS_STA_INDX, hot_sta_indx, STA::ExecuteINDX, CYC_INS_STA_INDX)                                                 \
    X(INS_STA_INDY, hot_sta_indy, STA::ExecuteINDY, CYC_INS_STA_INDY)                                                 \
    X(INS_STA_IND_ZP, hot_sta_ind_zp, STA::ExecuteINDZP, CYC_INS_STA_IND_ZP)                                         \
    X(INS_ADC_IM, hot_adc_im, ADC::ExecuteImmediate, CYC_INS_ADC_IM)                                                  \
    X(INS_ADC_ZP, hot_adc_zp, ADC::ExecuteZP, CYC_INS_ADC_ZP)                                                         \
    X(INS_ADC_ZPX, hot_adc_zpx, ADC::ExecuteZPX, CYC_INS_ADC_ZPX)                                                     \
    X(INS_ADC_ABS, hot_adc_abs, ADC::ExecuteABS, CYC_INS_ADC_ABS)                                                     \
    X(INS_ADC_ABSX, hot_adc_absx, ADC::ExecuteABSX, CYC_INS_ADC_ABSX)                                                 \
    X(INS_ADC_ABSY, hot_adc_absy, ADC::ExecuteABSY, CYC_INS_ADC_ABSY)                                                 \
    X(INS_ADC_INDX, hot_adc_indx, ADC::ExecuteINDX, CYC_INS_ADC_INDX)                                                 \
    X(INS_ADC_INDY, hot_adc_indy, ADC::ExecuteINDY, CYC_INS_ADC_INDY)                                                 \
    X(INS_ADC_IND_ZP, hot_adc_ind_zp, ADC::ExecuteIND_ZP, CYC_INS_ADC_IND_ZP)                                        \
    X(INS_SBC_IM, hot_sbc_im, SBC::ExecuteImmediate, CYC_INS_SBC_IM)                                                  \
    X(INS_SBC_ZP, hot_sbc_zp, SBC::ExecuteZP, CYC_INS_SBC_ZP)                                                         \
    X(INS_SBC_ZPX, hot_sbc_zpx, SBC::ExecuteZPX, CYC_INS_SBC_ZPX)                                                     \
    X(INS_SBC_ABS, hot_sbc_abs, SBC::ExecuteABS, CYC_INS_SBC_ABS)                                                     \
    X(INS_SBC_ABSX, hot_sbc_absx, SBC::ExecuteABSX, CYC_INS_SBC_ABSX)                                                 \
    X(INS_SBC_ABSY, hot_sbc_absy, SBC::ExecuteABSY, CYC_INS_SBC_ABSY)                                                 \
    X(INS_SBC_INDX, hot_sbc_indx, SBC::ExecuteINDX, CYC_INS_SBC_INDX)                                                 \
    X(INS_SBC_INDY, hot_sbc_indy, SBC::ExecuteINDY, CYC_INS_SBC_INDY)                                                 \
    X(INS_SBC_IND_ZP, hot_sbc_ind_zp, SBC::ExecuteIND_ZP, CYC_INS_SBC_IND_ZP)                                        \
    X(INS_BRA, hot_bra, BRA::Execute, CYC_INS_BRA)                                                                    \
    X(INS_BCC, hot_bcc, BCC::Execute, CYC_INS_BCC)                                                                    \
    X(INS_BCS, hot_bcs, BCS::Execute, CYC_INS_BCS)                                                                    \
    X(INS_BEQ, hot_beq, BEQ::Execute, CYC_INS_BEQ)                                                                    \
    X(INS_BNE, hot_bne, BNE::Execute, CYC_INS_BNE)                                                                    \
    X(INS_BMI, hot_bmi, BMI::Execute, CYC_INS_BMI)                                                                    \
    X(INS_BPL, hot_bpl, BPL::Execute, CYC_INS_BPL)                                                                    \
    X(INS_BVC, hot_bvc, BVC::Execute, CYC_INS_BVC)                                                                    \
    X(INS_BVS, hot_bvs, BVS::Execute, CYC_INS_BVS)                                                                    \
    X(INS_JMP_ABS, hot_jmp_abs, JMP::ExecuteABS, CYC_INS_JMP_ABS)                                                     \
    X(INS_JSR, hot_jsr, JSR::Execute, CYC_INS_JSR)                                                                    \
    X(INS_RTS, hot_rts, RTS::Execute, CYC_INS_RTS)                                                                    \
    X(INS_NOP, hot_nop, NOP::Execute, CYC_INS_NOP)

#if defined(__GNUC__) || defined(__clang__)
    // GNU/Clang fast path: direct-threaded jump table for common opcodes.
    static std::array<const void*, 256> hotTargets{};
    static bool hotTargetsInitialized = false;
    if (!hotTargetsInitialized) {
        hotTargets.fill(&&hot_slow_path);
#define SET_HOT_TARGET(opcodeValue, labelName, executor, cycles) hotTargets[opcodeValue] = &&labelName;
        HOT_OPCODE_LIST(SET_HOT_TARGET)
#undef SET_HOT_TARGET
        hotTargetsInitialized = true;
    }

    goto *hotTargets[opcode];

#define HOT_LABEL(opcodeValue, labelName, executor, cycles)                                                            \
    labelName:                                                                                                         \
        executor<Debug>(cpu, bus);                                                                                     \
        cpu.remainingCycles += cycles - 1;                                                                             \
        return 0;
    HOT_OPCODE_LIST(HOT_LABEL)
#undef HOT_LABEL
hot_slow_path:
#else
    // Portable fallback: fast switch for the same hot opcodes.
    switch (opcode) {
#define HOT_CASE(opcodeValue, labelName, executor, cycles)                                                             \
    case opcodeValue:                                                                                                  \
        executor<Debug>(cpu, bus);                                                                                     \
        cpu.remainingCycles += cycles - 1;                                                                             \
        return 0;
        HOT_OPCODE_LIST(HOT_CASE)
#undef HOT_CASE
        default:
            break;
    }
#endif

    const OpcodeEntry<Debug>& entry = dispatchTable<Debug>[opcode];

    // Execute instruction
    if (entry.executor) {
        entry.executor(cpu, bus);
    }

    // Assign cycle accuracy offsets (branchless equivalent to: baseCycles > 0 ? baseCycles - 1 : 0)
    cpu.remainingCycles += static_cast<int>(entry.baseCycles) - static_cast<int>(entry.baseCycles != 0);

    // Error handling
    if (entry.exitCode == -1) {
        std::cerr << "Unknown opcode: 0x" << std::hex << static_cast<int>(opcode) << " PC: 0x" << cpu.PC << std::dec
                  << " execution cancelled.\n";
    }

    return entry.exitCode;

#undef HOT_OPCODE_LIST
}

}  // namespace Hardware::CPUDispatch
