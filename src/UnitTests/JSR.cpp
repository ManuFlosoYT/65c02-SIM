#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class PruebaJSR : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(PruebaJSR, SaltaASubrutinaCorrecto) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_JSR);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8000, INS_JAM);

    Word PC_Inicial = 0x4000;
    Word PC_RetornoEsperado = PC_Inicial + 2;
    Word SP_Inicial = cpu.SP;

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x8001);
    EXPECT_EQ(cpu.SP, SP_Inicial - 2);

    Byte RetornoHigh = bus.ReadDirect(SP_Inicial);
    Byte RetornoLow = bus.ReadDirect(SP_Inicial - 1);

    EXPECT_EQ(RetornoHigh, (PC_RetornoEsperado >> 8) & 0xFF);
    EXPECT_EQ(RetornoLow, PC_RetornoEsperado & 0xFF);
}

TEST_F(PruebaJSR, JSRAnidado) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_JSR);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);

    bus.WriteDirect(0x8000, INS_JSR);
    bus.WriteDirect(0x8001, 0x00);
    bus.WriteDirect(0x8002, 0x90);

    bus.WriteDirect(0x9000, INS_JAM);

    Word SP_Inicial = cpu.SP;

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x9001);
    EXPECT_EQ(cpu.SP, SP_Inicial - 4);

    EXPECT_EQ(bus.ReadDirect(cpu.SP + 1), 0x02);
    EXPECT_EQ(bus.ReadDirect(cpu.SP + 2), 0x80);
    EXPECT_EQ(bus.ReadDirect(cpu.SP + 3), 0x02);
    EXPECT_EQ(bus.ReadDirect(cpu.SP + 4), 0x40);
}