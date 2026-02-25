#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STP_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STP_Test, STP_HaltsExecution) {
    // 0xFFFC: STP
    // 0xFFFD: NOP (Should not be executed)
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STP);
    bus.Write(0x4001, INS_NOP);

    // Run execution
    cpu.Execute(bus);

    // PC should point to the instruction AFTER STP (it fetches STP, increments,
    // executes). Wait, STP stops the clock. In a simulator loop, it should just
    // return. The PC usually points to the next byte but the fetch cycle for
    // the next instruction is never completed. If usage is `return` after
    // fetch: Fetch STP (PC increments to FFFD) Execute STP -> Return So PC
    // should be 0xFFFD.

    EXPECT_EQ(cpu.PC, 0x4001);
}