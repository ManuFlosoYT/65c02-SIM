#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STZ_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
    CPU cpu;
};

TEST_F(STZ_ZeroPage_Test, STZ_ZeroPage_ExecutesCorrectly) {
    // Programa en memoria:
    // 0xFFFC: STZ (ZeroPage) 0x42
    // 0xFFFD: 0x42
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    // 0x0042: 0xAA (Valor inicial)

    cpu.PC = 0xFFFC;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STZ_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x4002, INS_JAM);  // Stop

    // Set initial value to non-zero
    mem.Write(0x0042, 0xAA);

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x00);
    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(STZ_ZeroPage_Test, STZ_ZeroPage_DoesNotAffectFlags) {
    // STZ no afecta flags
    cpu.PC = 0xFFFC;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STZ_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x4002, INS_JAM);

    mem.Write(0x0042, 0xAA);

    // Set some flags
    cpu.Z = 0;
    cpu.N = 1;
    cpu.C = 1;
    cpu.V = 1;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x00);
    EXPECT_EQ(cpu.Z, 0);  // Should remain 0
    EXPECT_EQ(cpu.N, 1);  // Should remain 1
    EXPECT_EQ(cpu.C, 1);
    EXPECT_EQ(cpu.V, 1);
}