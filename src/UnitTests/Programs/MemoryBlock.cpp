#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class Programs_MemoryBlock : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(Programs_MemoryBlock, CopyBlock) {

    Word SRC_PAGE = 0x4000;
    Word DST_PAGE = 0x2000;
    Word CODE_START = 0x1000;

    for (int i = 0; i < 256; i++) {
        bus.Write(SRC_PAGE + i, (Byte)i);
        bus.Write(DST_PAGE + i, 0);
    }

    Word PC = CODE_START;

    bus.Write(PC++, INS_LDX_IM);
    bus.Write(PC++, 0x00);

    Word LOOP_START = PC;

    bus.Write(PC++, INS_LDA_ABSX);
    bus.Write(PC++, (Byte)(SRC_PAGE & 0xFF));
    bus.Write(PC++, (Byte)(SRC_PAGE >> 8));

    bus.Write(PC++, INS_STA_ABSX);
    bus.Write(PC++, (Byte)(DST_PAGE & 0xFF));
    bus.Write(PC++, (Byte)(DST_PAGE >> 8));

    bus.Write(PC++, INS_INX);

    bus.Write(PC++, INS_BNE);
    int8_t offset = LOOP_START - (PC + 1);
    bus.Write(PC++, (Byte)offset);

    bus.Write(PC++, INS_STP);

    cpu.PC = CODE_START;
    cpu.isInit =
        true;
    cpu.Execute(bus);

    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(bus.ReadDirect(DST_PAGE + i), (Byte)i) << "Fallo en indice " << i;
    }

    EXPECT_EQ(cpu.X, 0x00);
}