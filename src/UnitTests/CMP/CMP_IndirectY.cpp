#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class CMP_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(CMP_IndirectY_Test, CMP_IndirectY) {
    cpu.Y = 0x01;
    cpu.A = 0x05;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_INDY);
    bus.Write(0x4001, 0x02);
    bus.Write(0x0002, 0x00);
    bus.Write(0x0003, 0x80);
    bus.WriteDirect(0x8001, 0x05);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}