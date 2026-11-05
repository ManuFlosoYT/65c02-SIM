#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class CMP_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Equal) {
    cpu.A = 0x10;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_IND_ZP);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0005, 0x00);
    bus.Write(0x0006, 0x80);
    bus.WriteDirect(0x8000, 0x10);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Greater) {
    cpu.A = 0x20;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_IND_ZP);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);
    bus.Write(0x0011, 0x90);
    bus.WriteDirect(0x9000, 0x10);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x20);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Less) {
    cpu.A = 0x10;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_IND_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0020, 0x00);
    bus.Write(0x0021, 0xA0);
    bus.WriteDirect(0xA000, 0x20);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_PointerWrap) {
    cpu.A = 0x50;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_IND_ZP);
    bus.Write(0x4001, 0xFF);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x00FF, 0x10);
    bus.Write(0x0000, 0xB0);
    bus.WriteDirect(0xB010, 0x50);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x50);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}