#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ASL_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ASL_ZeroPageX_Test, ASL_ZeroPageX) {
    // Addr = 0x42 + 0x05 = 0x47
    // Mem[0x47] = 0x01 -> 0x02
    cpu.X = 0x05;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ASL_ZPX);
    bus.Write(0x4001, 0x42);
    bus.Write(0x0047, 0x01);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0047), 0x02);
}