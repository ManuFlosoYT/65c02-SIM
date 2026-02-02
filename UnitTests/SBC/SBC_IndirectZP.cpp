#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class SBC_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_IndirectZP_Test, SBC_IndirectZP_Basic) {
    cpu.A = 0x0A;
    cpu.C = 1;

    mem[0xFFFC] = INS_SBC_IND_ZP;
    mem[0xFFFD] = 0x02;     // ZP address
    mem[0xFFFE] = INS_JAM;  // Stop
    mem[0x0002] = 0x00;     // Pointer low
    mem[0x0003] = 0x80;     // Pointer high -> 0x8000
    mem[0x8000] = 0x05;     // Value

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
    EXPECT_TRUE(cpu.C);  // 10 - 5 = 5 >= 0, C=1 (No borrow)
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(SBC_IndirectZP_Test, SBC_IndirectZP_Borrow) {
    cpu.A = 0x05;
    cpu.C = 1;

    mem[0xFFFC] = INS_SBC_IND_ZP;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;  // Stop
    mem[0x0010] = 0x00;
    mem[0x0011] = 0x90;  // 0x9000
    mem[0x9000] = 0x0A;  // 5 - 10

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xFB);  // 5 - 10 = -5 = 0xFB
    EXPECT_FALSE(cpu.C);     // Borrow
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(SBC_IndirectZP_Test, SBC_IndirectZP_PointerWrap) {  // ZP = 0xFF
    cpu.A = 0x20;
    cpu.C = 1;

    mem[0xFFFC] = INS_SBC_IND_ZP;
    mem[0xFFFD] = 0xFF;     // ZP address at boundary
    mem[0xFFFE] = INS_JAM;  // Stop
    mem[0x00FF] = 0x10;     // Low byte at 0xFF
    mem[0x0000] = 0xA0;     // High byte at 0x00 (wrapped) -> 0xA010
    mem[0xA010] = 0x10;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x10);  // 0x20 - 0x10 = 0x10
    EXPECT_TRUE(cpu.C);
}
