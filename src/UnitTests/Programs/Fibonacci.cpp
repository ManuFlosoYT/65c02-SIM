#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class Programs_Fibonacci : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(Programs_Fibonacci, FibonacciSeries) {

    Byte N = 10;
    Word ADDR_SERIES = 0x0000;

    bus.Write(ADDR_SERIES, 0);
    bus.Write(ADDR_SERIES + 1, 1);

    Word CODE_START = 0x1000;
    Word PC = CODE_START;

    bus.Write(PC++, INS_LDX_IM);
    bus.Write(PC++, 0x00);

    bus.Write(PC++, INS_LDY_IM);
    bus.Write(PC++, N - 2);

    Word LOOP_START = PC;
    bus.Write(PC++, INS_CLC);

    bus.Write(PC++, INS_LDA_ZPX);
    bus.Write(PC++, (Byte)ADDR_SERIES);

    bus.Write(PC++, INS_INX);

    bus.Write(PC++, INS_ADC_ZPX);
    bus.Write(PC++, (Byte)ADDR_SERIES);

    bus.Write(PC++, INS_INX);

    bus.Write(PC++, INS_STA_ZPX);
    bus.Write(PC++, (Byte)ADDR_SERIES);

    bus.Write(PC++, INS_DEX);

    bus.Write(PC++, INS_DEY);

    bus.Write(PC++, INS_BNE);
    int8_t offset = LOOP_START - (PC + 1);
    bus.Write(PC++, (Byte)offset);

    bus.Write(PC++, INS_STP);

    cpu.PC = CODE_START;
    cpu.isInit = true;
    // Safety cycle bound to prevent infinite loop on branch failure
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    cpu.Execute(bus);

    Byte expected[] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34};
    for (int i = 0; i < N; i++) {
        EXPECT_EQ(bus.ReadDirect(ADDR_SERIES + i), expected[i]) << "Error en indice " << i;
    }
}