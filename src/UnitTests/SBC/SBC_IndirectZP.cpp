#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class SBC_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(SBC_IndirectZP_Test, SBC_IndirectZP_Basic) {
    cpu.A = 0x0A;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IND_ZP);
    bus.Write(0x4001, 0x02);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0002, 0x00);
    bus.Write(0x0003, 0x80);
    bus.WriteDirect(0x8000, 0x05);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x05);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(SBC_IndirectZP_Test, SBC_IndirectZP_Borrow) {
    cpu.A = 0x05;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IND_ZP);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);
    bus.Write(0x0011, 0x90);
    bus.WriteDirect(0x9000, 0x0A);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0xFB);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(SBC_IndirectZP_Test, SBC_IndirectZP_PointerWrap) {
    cpu.A = 0x20;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IND_ZP);
    bus.Write(0x4001, 0xFF);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x00FF, 0x10);
    bus.Write(0x0000, 0xA0);
    bus.WriteDirect(0xA010, 0x10);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_TRUE(cpu.C);
}