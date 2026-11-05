#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class Programs_Multiplication : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(Programs_Multiplication, SimpleMultiplication) {

    Byte ADDR_RES = 0x00;
    Byte ADDR_MUL1 = 0x01;
    Byte ADDR_MUL2 = 0x02;

    bus.Write(ADDR_RES, 0x00);
    bus.Write(ADDR_MUL1, 10);
    bus.Write(ADDR_MUL2, 5);

    Word CODE_START = 0x1000;
    Word PC = CODE_START;

    bus.Write(PC++, INS_LDA_IM);
    bus.Write(PC++, 0x00);

    bus.Write(PC++, INS_LDX_ZP);
    bus.Write(PC++, ADDR_MUL2);

    Word LOOP_START = PC;
    bus.Write(PC++, INS_CLC);

    bus.Write(PC++, INS_ADC_ZP);
    bus.Write(PC++, ADDR_MUL1);

    bus.Write(PC++, INS_DEX);

    bus.Write(PC++, INS_BNE);
    int8_t offset = LOOP_START - (PC + 1);
    bus.Write(PC++, (Byte)offset);

    bus.Write(PC++, INS_STA_ZP);
    bus.Write(PC++, ADDR_RES);

    bus.Write(PC++, INS_STP);

    cpu.PC = CODE_START;
    cpu.isInit = true;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(ADDR_RES), 50);
    EXPECT_EQ(cpu.X, 0);
}