#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class AND_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(AND_Immediate_Test, AND_Immediate) {
    // Programa en memoria:
    // 0xFFFC: AND #0x0F
    // 0xFFFD: 0x67
    // 0xFFFE: Opcode desconocido (0xFF)
    cpu.A = 0xFF;  // Start with A = 0xFF
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_AND_IM);
    bus.Write(0x4001, 0x0F);
    bus.Write(0x4002, INS_JAM);

    // Ciclo 1: Lee AND #0x0F
    // Ciclo 2: Lee 0x0F, ejecuta AND
    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x0F);  // 0xFF & 0x0F = 0x0F
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_Immediate_Test, AND_Immediate_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xF0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_AND_IM);
    bus.Write(0x4001, 0x0F);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);  // 0xF0 & 0x0F = 0x00
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_Immediate_Test, AND_Immediate_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0xFF;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_AND_IM);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x80);  // 0xFF & 0x80 = 0x80
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}