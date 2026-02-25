#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class RMB_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(RMB_Test, RMB0_ClearsBit0) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RMB0);
    bus.Write(0x4001, 0x10);     // Zero Page Address
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x0010, 0xFF);     // Initial value: 1111 1111

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0xFE);  // Expected: 1111 1110 (Bit 0 cleared)
}

TEST_F(RMB_Test, RMB1_ClearsBit1) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RMB1);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);
    bus.Write(0x0010, 0xFF);  // 1111 1111

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0xFD);  // 1111 1101 (Bit 1 cleared)
}

TEST_F(RMB_Test, RMB2_ClearsBit2) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RMB2);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);
    bus.Write(0x0010, 0xFF);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0xFB);  // 1111 1011
}

TEST_F(RMB_Test, RMB3_ClearsBit3) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RMB3);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);
    bus.Write(0x0010, 0xFF);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0xF7);  // 1111 0111
}

TEST_F(RMB_Test, RMB4_ClearsBit4) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RMB4);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);
    bus.Write(0x0010, 0xFF);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0xEF);  // 1110 1111
}

TEST_F(RMB_Test, RMB5_ClearsBit5) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RMB5);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);
    bus.Write(0x0010, 0xFF);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0xDF);  // 1101 1111
}

TEST_F(RMB_Test, RMB6_ClearsBit6) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RMB6);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);
    bus.Write(0x0010, 0xFF);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0xBF);  // 1011 1111
}

TEST_F(RMB_Test, RMB7_ClearsBit7) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RMB7);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);
    bus.Write(0x0010, 0xFF);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x7F);  // 0111 1111
}