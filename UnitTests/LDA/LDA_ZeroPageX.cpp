#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class LDA_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_ZeroPageX_Test, LDA_ZeroPageX) {
    // Programa en memoria:
    // 0xFFFC: LDA (ZeroPageX) 0x42
    // 0xFFFD: 0x42
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    // X register: 0x05
    // Dirección objetivo: 0x42 + 0x05 = 0x47
    // 0x0047: 0x37 (Valor a cargar)
    cpu.X = 0x05;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0047, 0x37);
    mem.Write(0x4002, INS_JAM);

    // Ciclo 1:
    //    Lee LDA (ZPX) en 0xFFFC
    //    PC avanza a 0xFFFD
    //    Ejecuta LDA (ZPX)
    // Ciclo 2:
    //    Lee la dirección base ZP (0x42) en 0xFFFD
    //    PC avanza a 0xFFFE
    // Ciclo 3:
    //    Lee dirección base 0x42
    //    Calcula dirección efectiva 0x42 + X (0x05) = 0x47
    // Ciclo 4:
    //    Lee el valor (0x37) en 0x0047
    //    Carga 0x37 en A
    //    Opcode desconocido -> Retorna
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_ZeroPageX_Test, LDA_ZeroPageX_ZeroFlag) {
    cpu.X = 0x05;
    cpu.Z = 0;
    cpu.A = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0047, 0x00);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_ZeroPageX_Test, LDA_ZeroPageX_NegativeFlag) {
    cpu.X = 0x05;
    cpu.N = 0;
    cpu.A = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0047, 0x88);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x88);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(LDA_ZeroPageX_Test, LDA_ZeroPageX_Wrapping) {
    cpu.X = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ZPX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x007F, 0x42);
    mem.Write(0x017F, 0xAD);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x42);
}