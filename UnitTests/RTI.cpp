#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class RTI_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(RTI_Test, RTI_Operations) {
    // RTI: Return from Interrupt
    // Pull Status (P), Pull PC.

    // Fake Stack
    cpu.SP = 0xFC;  // Stack Pointer pointing to empty
    // Pushed values (e.g. from BRK)
    mem[0x0100 + 0xFD] = 0b11000000;  // Status (N=1, V=1, others 0)
    mem[0x0100 + 0xFE] = 0x02;        // PC Low
    mem[0x0100 + 0xFF] = 0x10;        // PC High

    mem[0xFFFC] = INS_RTI;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1002);
    EXPECT_TRUE(cpu.N);
    EXPECT_TRUE(cpu.V);
    EXPECT_EQ(cpu.SP, 0xFF);
}
