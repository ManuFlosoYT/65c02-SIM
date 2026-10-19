#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Disassembler.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class CDL_Disassembler_Test : public ::testing::Test {
   public:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(CDL_Disassembler_Test, OpcodeAndOperandLogging) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);

    bus.Write(0x4000, INS_LDA_IM);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_NE(bus.GetCDL(0x4000) & CDL_OPCODE, 0);
    EXPECT_NE(bus.GetCDL(0x4001) & CDL_OPERAND, 0);

    DisassembledInstruction opInst = Disassembler::Disassemble(bus, 0x4000);
    EXPECT_EQ(opInst.mnemonic, "LDA");
    EXPECT_EQ(opInst.size, 2);

    DisassembledInstruction operandInst = Disassembler::Disassemble(bus, 0x4001);
    EXPECT_EQ(operandInst.mnemonic, ".byte");
    EXPECT_EQ(operandInst.operands, "$10");
    EXPECT_EQ(operandInst.size, 1);
}

TEST_F(CDL_Disassembler_Test, DataLoggingPreventsFakeInstructions) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);

    // 0x4C is INS_JMP_ABS, which normally would decode as a 3-byte JMP instruction
    bus.Write(0x5000, 0x4C);
    bus.Write(0x5001, 0x00);
    bus.Write(0x5002, 0x80);

    bus.Write(0x4000, INS_LDA_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x50);
    bus.Write(0x4003, INS_STP);

    cpu.Execute(bus);

    EXPECT_NE(bus.GetCDL(0x5000) & CDL_DATA, 0);
    EXPECT_EQ(bus.GetCDL(0x5000) & CDL_OPCODE, 0);

    DisassembledInstruction dataInst = Disassembler::Disassemble(bus, 0x5000);
    EXPECT_EQ(dataInst.mnemonic, ".byte");
    EXPECT_EQ(dataInst.operands, "$4C");
    EXPECT_EQ(dataInst.size, 1);
}

TEST_F(CDL_Disassembler_Test, ZeroPagePointerLogging) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);

    bus.Write(0x0020, 0x00);
    bus.Write(0x0021, 0x60);
    bus.Write(0x6000, 0x99);

    bus.Write(0x4000, INS_LDA_IND_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_NE(bus.GetCDL(0x0020) & CDL_ZP_POINTER, 0);
    EXPECT_NE(bus.GetCDL(0x0021) & CDL_ZP_POINTER, 0);
    EXPECT_NE(bus.GetCDL(0x6000) & CDL_DATA, 0);

    DisassembledInstruction zpInst0 = Disassembler::Disassemble(bus, 0x0020);
    EXPECT_EQ(zpInst0.mnemonic, ".byte");
    EXPECT_EQ(zpInst0.operands, "$00");
    EXPECT_EQ(zpInst0.size, 1);

    DisassembledInstruction zpInst1 = Disassembler::Disassemble(bus, 0x0021);
    EXPECT_EQ(zpInst1.mnemonic, ".byte");
    EXPECT_EQ(zpInst1.operands, "$60");
    EXPECT_EQ(zpInst1.size, 1);
}

TEST_F(CDL_Disassembler_Test, UnexecutedCodeFallbackAndClearCDL) {
    bus.Write(0x7000, INS_NOP);
    EXPECT_EQ(bus.GetCDL(0x7000), CDL_NONE);

    DisassembledInstruction unexecutedInst = Disassembler::Disassemble(bus, 0x7000);
    EXPECT_EQ(unexecutedInst.mnemonic, "NOP");
    EXPECT_EQ(unexecutedInst.size, 1);

    bus.LogCDL(0x7000, CDL_DATA);
    EXPECT_EQ(bus.GetCDL(0x7000), CDL_DATA);

    bus.ClearCDL();
    EXPECT_EQ(bus.GetCDL(0x7000), CDL_NONE);
}
