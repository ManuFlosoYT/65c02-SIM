#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ADC_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ADC_IndirectY_Test, ADC_IndirectY) {
    // ZP Addr = 0x02
    // Mem[0x02] = 0x00, Mem[0x03] = 0x80 -> Base Addr = 0x8000
    // Addr = 0x8000 + 0x01 = 0x8001
    // Mem[0x8001] = 0x37
    cpu.Y = 0x01;
    cpu.A = 0x00;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_INDY);
    bus.Write(0x4001, 0x02);
    bus.Write(0x0002, 0x00);
    bus.Write(0x0003, 0x80);
    bus.WriteDirect(0x8001, 0x37);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x37);
}