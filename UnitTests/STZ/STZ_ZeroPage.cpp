#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STZ_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STZ_ZeroPage_Test, STZ_ZeroPage_ExecutesCorrectly) {
    // Programa en memoria:
    // 0xFFFC: STZ (ZeroPage) 0x42
    // 0xFFFD: 0x42
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    // 0x0042: 0xAA (Valor inicial)

    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STZ_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x4002, INS_JAM);  // Stop

    // Set initial value to non-zero
    bus.Write(0x0042, 0xAA);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x00);
    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(STZ_ZeroPage_Test, STZ_ZeroPage_DoesNotAffectFlags) {
    // STZ no afecta flags
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STZ_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x4002, INS_JAM);

    bus.Write(0x0042, 0xAA);

    // Set some flags
    cpu.Z = 0;
    cpu.N = 1;
    cpu.C = 1;
    cpu.V = 1;

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x00);
    EXPECT_EQ(cpu.Z, 0);  // Should remain 0
    EXPECT_EQ(cpu.N, 1);  // Should remain 1
    EXPECT_EQ(cpu.C, 1);
    EXPECT_EQ(cpu.V, 1);
}