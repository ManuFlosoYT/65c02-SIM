#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class RMB_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(RMB_Test, RMB0_ClearsBit0) {
    mem[0xFFFC] = INS_RMB0;
    mem[0xFFFD] = 0x10;  // Zero Page Address
    mem[0xFFFE] = 0xFF;  // Stop
    mem[0x0010] = 0xFF;  // Initial value: 1111 1111

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0xFE);  // Expected: 1111 1110 (Bit 0 cleared)
}

TEST_F(RMB_Test, RMB1_ClearsBit1) {
    mem[0xFFFC] = INS_RMB1;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = 0xFF;
    mem[0x0010] = 0xFF;  // 1111 1111

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0xFD);  // 1111 1101 (Bit 1 cleared)
}

TEST_F(RMB_Test, RMB2_ClearsBit2) {
    mem[0xFFFC] = INS_RMB2;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = 0xFF;
    mem[0x0010] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0xFB);  // 1111 1011
}

TEST_F(RMB_Test, RMB3_ClearsBit3) {
    mem[0xFFFC] = INS_RMB3;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = 0xFF;
    mem[0x0010] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0xF7);  // 1111 0111
}

TEST_F(RMB_Test, RMB4_ClearsBit4) {
    mem[0xFFFC] = INS_RMB4;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = 0xFF;
    mem[0x0010] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0xEF);  // 1110 1111
}

TEST_F(RMB_Test, RMB5_ClearsBit5) {
    mem[0xFFFC] = INS_RMB5;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = 0xFF;
    mem[0x0010] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0xDF);  // 1101 1111
}

TEST_F(RMB_Test, RMB6_ClearsBit6) {
    mem[0xFFFC] = INS_RMB6;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = 0xFF;
    mem[0x0010] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0xBF);  // 1011 1111
}

TEST_F(RMB_Test, RMB7_ClearsBit7) {
    mem[0xFFFC] = INS_RMB7;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = 0xFF;
    mem[0x0010] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x7F);  // 0111 1111
}
