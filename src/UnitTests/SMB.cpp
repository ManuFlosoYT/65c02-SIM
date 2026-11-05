#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class SMB_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};


TEST_F(SMB_Test, SMB0_SetsBit0) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB0);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x01);
}

TEST_F(SMB_Test, SMB0_AlreadySet) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB0);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x01);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010),
              0x01);
}

TEST_F(SMB_Test, SMB1_SetsBit1) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB1);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x02);
}

TEST_F(SMB_Test, SMB2_SetsBit2) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB2);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x04);
}

TEST_F(SMB_Test, SMB3_SetsBit3) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB3);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x08);
}

TEST_F(SMB_Test, SMB4_SetsBit4) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB4);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x10);
}

TEST_F(SMB_Test, SMB5_SetsBit5) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB5);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x20);
}

TEST_F(SMB_Test, SMB6_SetsBit6) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB6);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x40);
}

TEST_F(SMB_Test, SMB7_SetsBit7) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SMB7);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_STP);
    bus.Write(0x0010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0010), 0x80);
}