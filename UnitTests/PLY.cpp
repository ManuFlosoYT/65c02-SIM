#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/ListaInstrucciones.h"

class PLY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PLY_Test, PLY) {
    // 0xFFFC: PLY
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_PLY);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x01FE;
    mem.Write(0x01FF, 0x42);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x42);
    EXPECT_EQ(cpu.SP, 0x01FF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(PLY_Test, PLY_ZeroFlag) {
    // 0xFFFC: PLY
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_PLY);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x01FE;
    mem.Write(0x01FF, 0x00);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(PLY_Test, PLY_NegativeFlag) {
    // 0xFFFC: PLY
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_PLY);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x01FE;
    mem.Write(0x01FF, 0x80);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}