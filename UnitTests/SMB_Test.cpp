#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class SMB_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// SMB should SET the bit to 1. 0 -> 1, 1 -> 1.

TEST_F(SMB_Test, SMB0_SetsBit0) {
    mem[0xFFFC] = INS_SMB0;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;  // Stop
    mem[0x0010] = 0x00;  // 0000 0000

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x01);  // 0000 0001
}

TEST_F(SMB_Test, SMB0_AlreadySet) {
    mem[0xFFFC] = INS_SMB0;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;  // Stop
    mem[0x0010] = 0x01;  // 0000 0001

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010],
              0x01);  // Should stay 0000 0001. If XOR, it would become 0x00.
}

TEST_F(SMB_Test, SMB1_SetsBit1) {
    mem[0xFFFC] = INS_SMB1;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;
    mem[0x0010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x02);  // 0000 0010
}

TEST_F(SMB_Test, SMB2_SetsBit2) {
    mem[0xFFFC] = INS_SMB2;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;
    mem[0x0010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x04);  // 0000 0100
}

TEST_F(SMB_Test, SMB3_SetsBit3) {
    mem[0xFFFC] = INS_SMB3;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;
    mem[0x0010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x08);  // 0000 1000
}

TEST_F(SMB_Test, SMB4_SetsBit4) {
    mem[0xFFFC] = INS_SMB4;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;
    mem[0x0010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x10);  // 0001 0000
}

TEST_F(SMB_Test, SMB5_SetsBit5) {
    mem[0xFFFC] = INS_SMB5;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;
    mem[0x0010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x20);  // 0010 0000
}

TEST_F(SMB_Test, SMB6_SetsBit6) {
    mem[0xFFFC] = INS_SMB6;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;
    mem[0x0010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x40);  // 0100 0000
}

TEST_F(SMB_Test, SMB7_SetsBit7) {
    mem[0xFFFC] = INS_SMB7;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;
    mem[0x0010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0010], 0x80);  // 1000 0000
}
