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
    table[INS_JMP_IND] = {&JMP::ExecuteIND<Debug>, CYC_INS_JMP_IND, 0};
    table[INS_JMP_ABSX] = {&JMP::ExecuteABSX<Debug>, CYC_INS_JMP_ABSX, 0};
    table[INS_PHP] = {&PHP::Execute<Debug>, CYC_INS_PHP, 0};
    table[INS_PLP] = {&PLP::Execute<Debug>, CYC_INS_PLP, 0};
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
    table[INS_BRK] = {&BRK::Execute<Debug>, CYC_INS_BRK, 0};
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
    X(INS_LDA_IM, LDA::ExecuteImmediate, CYC_INS_LDA_IM)                                                               \
    X(INS_LDA_ZP, LDA::ExecuteZP, CYC_INS_LDA_ZP)                                                                      \
    X(INS_LDA_ZPX, LDA::ExecuteZPX, CYC_INS_LDA_ZPX)                                                                   \
    X(INS_LDA_ABS, LDA::ExecuteABS, CYC_INS_LDA_ABS)                                                                   \
    X(INS_LDA_ABSX, LDA::ExecuteABSX, CYC_INS_LDA_ABSX)                                                                \
    X(INS_LDA_ABSY, LDA::ExecuteABSY, CYC_INS_LDA_ABSY)                                                                \
    X(INS_LDA_INDX, LDA::ExecuteINDX, CYC_INS_LDA_INDX)                                                                \
    X(INS_LDA_INDY, LDA::ExecuteINDY, CYC_INS_LDA_INDY)                                                                \
    X(INS_LDA_IND_ZP, LDA::ExecuteIND_ZP, CYC_INS_LDA_IND_ZP)                                                          \
    X(INS_LDX_IM, LDX::ExecuteImmediate, CYC_INS_LDX_IM)                                                               \
    X(INS_LDX_ZP, LDX::ExecuteZP, CYC_INS_LDX_ZP)                                                                      \
    X(INS_LDX_ZPY, LDX::ExecuteZPY, CYC_INS_LDX_ZPY)                                                                   \
    X(INS_LDX_ABS, LDX::ExecuteABS, CYC_INS_LDX_ABS)                                                                   \
    X(INS_LDX_ABSY, LDX::ExecuteABSY, CYC_INS_LDX_ABSY)                                                                \
    X(INS_LDY_IM, LDY::ExecuteImmediate, CYC_INS_LDY_IM)                                                               \
    X(INS_LDY_ZP, LDY::ExecuteZP, CYC_INS_LDY_ZP)                                                                      \
    X(INS_LDY_ZPX, LDY::ExecuteZPX, CYC_INS_LDY_ZPX)                                                                   \
    X(INS_LDY_ABS, LDY::ExecuteABS, CYC_INS_LDY_ABS)                                                                   \
    X(INS_LDY_ABSX, LDY::ExecuteABSX, CYC_INS_LDY_ABSX)                                                                \
    X(INS_STA_ZP, STA::ExecuteZP, CYC_INS_STA_ZP)                                                                      \
    X(INS_STA_ZPX, STA::ExecuteZPX, CYC_INS_STA_ZPX)                                                                   \
    X(INS_STA_ABS, STA::ExecuteABS, CYC_INS_STA_ABS)                                                                   \
    X(INS_STA_ABSX, STA::ExecuteABSX, CYC_INS_STA_ABSX)                                                                \
    X(INS_STA_ABSY, STA::ExecuteABSY, CYC_INS_STA_ABSY)                                                                \
    X(INS_STA_INDX, STA::ExecuteINDX, CYC_INS_STA_INDX)                                                                \
    X(INS_STA_INDY, STA::ExecuteINDY, CYC_INS_STA_INDY)                                                                \
    X(INS_STA_IND_ZP, STA::ExecuteINDZP, CYC_INS_STA_IND_ZP)                                                           \
    X(INS_STX_ZP, STX::ExecuteZP, CYC_INS_STX_ZP)                                                                      \
    X(INS_STX_ZPY, STX::ExecuteZPY, CYC_INS_STX_ZPY)                                                                   \
    X(INS_STX_ABS, STX::ExecuteABS, CYC_INS_STX_ABS)                                                                   \
    X(INS_STY_ZP, STY::ExecuteZP, CYC_INS_STY_ZP)                                                                      \
    X(INS_STY_ZPX, STY::ExecuteZPX, CYC_INS_STY_ZPX)                                                                   \
    X(INS_STY_ABS, STY::ExecuteABS, CYC_INS_STY_ABS)                                                                   \
    X(INS_STZ_ZP, STZ::ExecuteZP, CYC_INS_STZ_ZP)                                                                      \
    X(INS_STZ_ZPX, STZ::ExecuteZPX, CYC_INS_STZ_ZPX)                                                                   \
    X(INS_STZ_ABS, STZ::ExecuteABS, CYC_INS_STZ_ABS)                                                                   \
    X(INS_STZ_ABSX, STZ::ExecuteABSX, CYC_INS_STZ_ABSX)                                                                \
    X(INS_ADC_IM, ADC::ExecuteImmediate, CYC_INS_ADC_IM)                                                               \
    X(INS_ADC_ZP, ADC::ExecuteZP, CYC_INS_ADC_ZP)                                                                      \
    X(INS_ADC_ZPX, ADC::ExecuteZPX, CYC_INS_ADC_ZPX)                                                                   \
    X(INS_ADC_ABS, ADC::ExecuteABS, CYC_INS_ADC_ABS)                                                                   \
    X(INS_ADC_ABSX, ADC::ExecuteABSX, CYC_INS_ADC_ABSX)                                                                \
    X(INS_ADC_ABSY, ADC::ExecuteABSY, CYC_INS_ADC_ABSY)                                                                \
    X(INS_ADC_INDX, ADC::ExecuteINDX, CYC_INS_ADC_INDX)                                                                \
    X(INS_ADC_INDY, ADC::ExecuteINDY, CYC_INS_ADC_INDY)                                                                \
    X(INS_ADC_IND_ZP, ADC::ExecuteIND_ZP, CYC_INS_ADC_IND_ZP)                                                          \
    X(INS_SBC_IM, SBC::ExecuteImmediate, CYC_INS_SBC_IM)                                                               \
    X(INS_SBC_ZP, SBC::ExecuteZP, CYC_INS_SBC_ZP)                                                                      \
    X(INS_SBC_ZPX, SBC::ExecuteZPX, CYC_INS_SBC_ZPX)                                                                   \
    X(INS_SBC_ABS, SBC::ExecuteABS, CYC_INS_SBC_ABS)                                                                   \
    X(INS_SBC_ABSX, SBC::ExecuteABSX, CYC_INS_SBC_ABSX)                                                                \
    X(INS_SBC_ABSY, SBC::ExecuteABSY, CYC_INS_SBC_ABSY)                                                                \
    X(INS_SBC_INDX, SBC::ExecuteINDX, CYC_INS_SBC_INDX)                                                                \
    X(INS_SBC_INDY, SBC::ExecuteINDY, CYC_INS_SBC_INDY)                                                                \
    X(INS_SBC_IND_ZP, SBC::ExecuteIND_ZP, CYC_INS_SBC_IND_ZP)                                                          \
    X(INS_AND_IM, AND::ExecuteImmediate, CYC_INS_AND_IM)                                                               \
    X(INS_AND_ZP, AND::ExecuteZP, CYC_INS_AND_ZP)                                                                      \
    X(INS_AND_ZPX, AND::ExecuteZPX, CYC_INS_AND_ZPX)                                                                   \
    X(INS_AND_ABS, AND::ExecuteABS, CYC_INS_AND_ABS)                                                                   \
    X(INS_AND_ABSX, AND::ExecuteABSX, CYC_INS_AND_ABSX)                                                                \
    X(INS_AND_ABSY, AND::ExecuteABSY, CYC_INS_AND_ABSY)                                                                \
    X(INS_AND_INDX, AND::ExecuteINDX, CYC_INS_AND_INDX)                                                                \
    X(INS_AND_INDY, AND::ExecuteINDY, CYC_INS_AND_INDY)                                                                \
    X(INS_AND_IND_ZP, AND::ExecuteIND_ZP, CYC_INS_AND_IND_ZP)                                                          \
    X(INS_ORA_IM, ORA::ExecuteImmediate, CYC_INS_ORA_IM)                                                               \
    X(INS_ORA_ZP, ORA::ExecuteZP, CYC_INS_ORA_ZP)                                                                      \
    X(INS_ORA_ZPX, ORA::ExecuteZPX, CYC_INS_ORA_ZPX)                                                                   \
    X(INS_ORA_ABS, ORA::ExecuteABS, CYC_INS_ORA_ABS)                                                                   \
    X(INS_ORA_ABSX, ORA::ExecuteABSX, CYC_INS_ORA_ABSX)                                                                \
    X(INS_ORA_ABSY, ORA::ExecuteABSY, CYC_INS_ORA_ABSY)                                                                \
    X(INS_ORA_INDX, ORA::ExecuteINDX, CYC_INS_ORA_INDX)                                                                \
    X(INS_ORA_INDY, ORA::ExecuteINDY, CYC_INS_ORA_INDY)                                                                \
    X(INS_ORA_IND_ZP, ORA::ExecuteIND_ZP, CYC_INS_ORA_IND_ZP)                                                          \
    X(INS_EOR_IM, EOR::ExecuteImmediate, CYC_INS_EOR_IM)                                                               \
    X(INS_EOR_ZP, EOR::ExecuteZP, CYC_INS_EOR_ZP)                                                                      \
    X(INS_EOR_ZPX, EOR::ExecuteZPX, CYC_INS_EOR_ZPX)                                                                   \
    X(INS_EOR_ABS, EOR::ExecuteABS, CYC_INS_EOR_ABS)                                                                   \
    X(INS_EOR_ABSX, EOR::ExecuteABSX, CYC_INS_EOR_ABSX)                                                                \
    X(INS_EOR_ABSY, EOR::ExecuteABSY, CYC_INS_EOR_ABSY)                                                                \
    X(INS_EOR_INDX, EOR::ExecuteINDX, CYC_INS_EOR_INDX)                                                                \
    X(INS_EOR_INDY, EOR::ExecuteINDY, CYC_INS_EOR_INDY)                                                                \
    X(INS_EOR_IND_ZP, EOR::ExecuteIND_ZP, CYC_INS_EOR_IND_ZP)                                                          \
    X(INS_BIT_IM, BIT::ExecuteImmediate, CYC_INS_BIT_IM)                                                               \
    X(INS_BIT_ZP, BIT::ExecuteZP, CYC_INS_BIT_ZP)                                                                      \
    X(INS_BIT_ZPX, BIT::ExecuteZPX, CYC_INS_BIT_ZPX)                                                                   \
    X(INS_BIT_ABS, BIT::ExecuteABS, CYC_INS_BIT_ABS)                                                                   \
    X(INS_BIT_ABSX, BIT::ExecuteABSX, CYC_INS_BIT_ABSX)                                                                \
    X(INS_CMP_IM, CMP::ExecuteImmediate, CYC_INS_CMP_IM)                                                               \
    X(INS_CMP_ZP, CMP::ExecuteZP, CYC_INS_CMP_ZP)                                                                      \
    X(INS_CMP_ZPX, CMP::ExecuteZPX, CYC_INS_CMP_ZPX)                                                                   \
    X(INS_CMP_ABS, CMP::ExecuteABS, CYC_INS_CMP_ABS)                                                                   \
    X(INS_CMP_ABSX, CMP::ExecuteABSX, CYC_INS_CMP_ABSX)                                                                \
    X(INS_CMP_ABSY, CMP::ExecuteABSY, CYC_INS_CMP_ABSY)                                                                \
    X(INS_CMP_INDX, CMP::ExecuteINDX, CYC_INS_CMP_INDX)                                                                \
    X(INS_CMP_INDY, CMP::ExecuteINDY, CYC_INS_CMP_INDY)                                                                \
    X(INS_CMP_IND_ZP, CMP::ExecuteIND_ZP, CYC_INS_CMP_IND_ZP)                                                          \
    X(INS_CPX_IM, CPX::ExecuteImmediate, CYC_INS_CPX_IM)                                                               \
    X(INS_CPX_ZP, CPX::ExecuteZP, CYC_INS_CPX_ZP)                                                                      \
    X(INS_CPX_ABS, CPX::ExecuteABS, CYC_INS_CPX_ABS)                                                                   \
    X(INS_CPY_IM, CPY::ExecuteImmediate, CYC_INS_CPY_IM)                                                               \
    X(INS_CPY_ZP, CPY::ExecuteZP, CYC_INS_CPY_ZP)                                                                      \
    X(INS_CPY_ABS, CPY::ExecuteABS, CYC_INS_CPY_ABS)                                                                   \
    X(INS_INC_A, INC::ExecuteAccumulator, CYC_INS_INC_A)                                                               \
    X(INS_INC_ZP, INC::ExecuteZP, CYC_INS_INC_ZP)                                                                      \
    X(INS_INC_ZPX, INC::ExecuteZPX, CYC_INS_INC_ZPX)                                                                   \
    X(INS_INC_ABS, INC::ExecuteABS, CYC_INS_INC_ABS)                                                                   \
    X(INS_INC_ABSX, INC::ExecuteABSX, CYC_INS_INC_ABSX)                                                                \
    X(INS_DEC_A, DEC::ExecuteAccumulator, CYC_INS_DEC_A)                                                               \
    X(INS_DEC_ZP, DEC::ExecuteZP, CYC_INS_DEC_ZP)                                                                      \
    X(INS_DEC_ZPX, DEC::ExecuteZPX, CYC_INS_DEC_ZPX)                                                                   \
    X(INS_DEC_ABS, DEC::ExecuteABS, CYC_INS_DEC_ABS)                                                                   \
    X(INS_DEC_ABSX, DEC::ExecuteABSX, CYC_INS_DEC_ABSX)                                                                \
    X(INS_INX, INX::Execute, CYC_INS_INX)                                                                              \
    X(INS_INY, INY::Execute, CYC_INS_INY)                                                                              \
    X(INS_DEX, DEX::Execute, CYC_INS_DEX)                                                                              \
    X(INS_DEY, DEY::Execute, CYC_INS_DEY)                                                                              \
    X(INS_TAX, TAX::Execute, CYC_INS_TAX)                                                                              \
    X(INS_TAY, TAY::Execute, CYC_INS_TAY)                                                                              \
    X(INS_TXA, TXA::Execute, CYC_INS_TXA)                                                                              \
    X(INS_TYA, TYA::Execute, CYC_INS_TYA)                                                                              \
    X(INS_TSX, TSX::Execute, CYC_INS_TSX)                                                                              \
    X(INS_TXS, TXS::Execute, CYC_INS_TXS)                                                                              \
    X(INS_PHA, PHA::Execute, CYC_INS_PHA)                                                                              \
    X(INS_PLA, PLA::Execute, CYC_INS_PLA)                                                                              \
    X(INS_PHX, PHX::Execute, CYC_INS_PHX)                                                                              \
    X(INS_PLX, PLX::Execute, CYC_INS_PLX)                                                                              \
    X(INS_PHY, PHY::Execute, CYC_INS_PHY)                                                                              \
    X(INS_PLY, PLY::Execute, CYC_INS_PLY)                                                                              \
    X(INS_CLC, CLC::Execute, CYC_INS_CLC)                                                                              \
    X(INS_SEC, SEC::Execute, CYC_INS_SEC)                                                                              \
    X(INS_CLD, CLD::Execute, CYC_INS_CLD)                                                                              \
    X(INS_SED, SED::Execute, CYC_INS_SED)                                                                              \
    X(INS_CLI, CLI::Execute, CYC_INS_CLI)                                                                              \
    X(INS_SEI, SEI::Execute, CYC_INS_SEI)                                                                              \
    X(INS_CLV, CLV::Execute, CYC_INS_CLV)                                                                              \
    X(INS_BRA, BRA::Execute, CYC_INS_BRA)                                                                              \
    X(INS_BCC, BCC::Execute, CYC_INS_BCC)                                                                              \
    X(INS_BCS, BCS::Execute, CYC_INS_BCS)                                                                              \
    X(INS_BEQ, BEQ::Execute, CYC_INS_BEQ)                                                                              \
    X(INS_BNE, BNE::Execute, CYC_INS_BNE)                                                                              \
    X(INS_BMI, BMI::Execute, CYC_INS_BMI)                                                                              \
    X(INS_BPL, BPL::Execute, CYC_INS_BPL)                                                                              \
    X(INS_BVC, BVC::Execute, CYC_INS_BVC)                                                                              \
    X(INS_BVS, BVS::Execute, CYC_INS_BVS)                                                                              \
    X(INS_JMP_ABS, JMP::ExecuteABS, CYC_INS_JMP_ABS)                                                                   \
    X(INS_JSR, JSR::Execute, CYC_INS_JSR)                                                                              \
    X(INS_RTS, RTS::Execute, CYC_INS_RTS)                                                                              \
    X(INS_NOP, NOP::Execute, CYC_INS_NOP)

    switch (opcode) {
#define HOT_CASE(opcodeValue, executor, cycles)                                                                        \
    case opcodeValue:                                                                                                  \
        executor<Debug>(cpu, bus);                                                                                     \
        cpu.remainingCycles += cycles - 1;                                                                             \
        return 0;
        HOT_OPCODE_LIST(HOT_CASE)
#undef HOT_CASE
        default:
            break;
    }

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
