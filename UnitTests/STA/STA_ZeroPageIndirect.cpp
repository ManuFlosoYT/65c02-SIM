#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STA_ZeroPageIndirect_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STA_ZeroPageIndirect_Test, STA_ZeroPageIndirect) {
    cpu.A = 0x37;

    // 0xFFFC: STA (ZeroPage Indirect) 0x20
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STA_IND_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x4002, INS_JAM);  // Stop

    // Pointer Address (ZP) = 0x20
    bus.Write(0x0020, 0x00);  // Low Byte
    bus.Write(0x0021, 0x30);  // High Byte
    // Effective Address = 0x3000

    bus.Write(0x3000, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x3000), 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}