#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class STA_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_ZeroPage_Test, STA_ZeroPage) {
    // Programa en memoria:
    // 0xFFFC: STA (ZeroPage) 0x42
    // 0xFFFD: 0x42
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    // 0x0042: 0x00 (Valor inicial)

    cpu.A = 0x37;  // Valor a guardar

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STA_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x00);
    mem.Write(0x4002, INS_JAM);

    // Ciclo 1:
    //    Lee STA (ZP) en 0xFFFC
    //    PC avanza a 0xFFFD
    //    Ejecuta STA (ZP)
    // Ciclo 2:
    //    Lee la dirección ZP (0x42) en 0xFFFD
    //    PC avanza a 0xFFFE
    // Ciclo 3:
    //    Escribe el valor de A (0x37) en 0x0042
    //    Opcode desconocido -> Retorna
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(mem[0x0042], 0x37);
    EXPECT_EQ(cpu.A, 0x37);  // A no debe cambiar
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}