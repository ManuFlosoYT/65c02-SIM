#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class SBC_Decimal_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(SBC_Decimal_Test, SBC_Decimal_Simple) {
    // 0x10 - 0x05 = 0x05 (BCD)
    cpu.A = 0x10;
    cpu.D = 1;
    cpu.C = 1;  // No borrow

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IM);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x05);
    EXPECT_TRUE(cpu.C);  // No borrow
}

TEST_F(SBC_Decimal_Test, SBC_Decimal_Borrow) {
    // 0x05 - 0x06 = 0x99 (BCD 105 - 6 = 99), C=0 (Borrow)
    cpu.A = 0x05;
    cpu.D = 1;
    cpu.C = 1;  // No borrow start

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IM);
    bus.Write(0x4001, 0x06);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x99);
    EXPECT_FALSE(cpu.C);  // Borrow
}