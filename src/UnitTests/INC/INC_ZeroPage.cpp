#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class INC_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(INC_ZeroPage_Test, INC_ZeroPage) {
    // Mem[0x42] = 0x05 -> 0x06
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INC_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x0042, 0x05);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INC_ZeroPage_Test, INC_ZeroPage_ZeroFlag) {
    // Mem[0x42] = 0xFF -> 0x00
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INC_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x0042, 0xFF);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INC_ZeroPage_Test, INC_ZeroPage_NegativeFlag) {
    // Mem[0x42] = 0x7F -> 0x80
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INC_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x0042, 0x7F);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}