#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ADC_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ADC_IndirectX_Test, ADC_IndirectX) {
    // ZP Addr = 0x02 + 0x04 = 0x06
    // Mem[0x06] = 0x00, Mem[0x07] = 0x80 -> Addr = 0x8000
    // Mem[0x8000] = 0x37
    cpu.X = 0x04;
    cpu.A = 0x00;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_INDX);
    bus.Write(0x4001, 0x02);
    bus.Write(0x0006, 0x00);
    bus.Write(0x0007, 0x80);
    bus.WriteDirect(0x8000, 0x37);  // 55
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x37);
}