#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class PruebaNOP : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(PruebaNOP, NoHaceNada) {
    // Programa en memoria:
    // 0xFFFC: NOP (0xEA)
    // 0xFFFD: Opcode desconocido (0xFF) para detener la ejecución
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_NOP);
    bus.Write(0x4001, INS_JAM);

    // Ciclo 1:
    //     Lee NOP en 0xFFFC
    //     PC avanza a 0xFFFD
    //     Ejecuta NOP
    // Ciclo 2:
    //     Lee 0xFF en 0xFFFD
    //     PC avanza a 0xFFFE
    //     Opcode desconocido
    //     Retorna
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4002);

    // Verify Registers are unchanged (Default 0 in Setup, but let's change them
    // before)
}

TEST_F(PruebaNOP, NoModificaEstado) {
    cpu.A = 0x42;
    cpu.X = 0x42;
    cpu.Y = 0x42;
    cpu.SP = 0xF0;
    cpu.C = 1;
    cpu.Z = 1;
    cpu.I = 1;
    cpu.D = 1;
    cpu.B = 1;
    cpu.V = 1;
    cpu.N = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_NOP);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4002);
    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_EQ(cpu.X, 0x42);
    EXPECT_EQ(cpu.Y, 0x42);
    EXPECT_EQ(cpu.SP, 0xF0);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.I);
    EXPECT_TRUE(cpu.D);
    EXPECT_TRUE(cpu.B);
    EXPECT_TRUE(cpu.V);
    EXPECT_TRUE(cpu.N);
}