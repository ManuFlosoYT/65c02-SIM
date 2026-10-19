#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/BreakpointManager.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BreakpointExpressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
    BreakpointManager bpManager;
};

TEST_F(BreakpointExpressionTest, Validation) {
    std::string err;
    EXPECT_FALSE(BreakpointManager::ValidateExpression("", &err));
    EXPECT_FALSE(BreakpointManager::ValidateExpression("PC == &&", &err));
    EXPECT_TRUE(BreakpointManager::ValidateExpression("PC == $8000 && A > $10 || MEM[$0200] changed", &err));
}

TEST_F(BreakpointExpressionTest, ComplexExpressionEvaluate) {
    Breakpoint bp;
    bp.label = "ComplexWatchpoint";
    BreakCondition cond;
    cond.type = BreakpointType::Expression;
    cond.expression = "PC == $8000 && A > $10 || MEM[$0200] changed";
    bp.conditions.push_back(cond);

    uint32_t id = bpManager.AddBreakpoint(std::move(bp));
    ASSERT_GT(id, 0U);

    // Initial state: PC = 0x1000, A = 0, MEM[$0200] = 0
    cpu.PC = 0x1000;
    cpu.A = 0x00;
    bus.WriteDirect(0x0200, 0x00);

    // First evaluation: baseline initialized, neither branch is true
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), 0U);

    // Set PC to $8000 but A is still <= $10 (0x05) -> not hit
    cpu.PC = 0x8000;
    cpu.A = 0x05;
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), 0U);

    // Set A to $15 (> $10) with PC == $8000 -> hit!
    cpu.A = 0x15;
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), id);

    // Change PC back to $1000 so left branch is false
    cpu.PC = 0x1000;
    cpu.A = 0x00;
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), 0U);

    // Modify memory at $0200 -> right branch (MEM[$0200] changed) becomes true!
    bus.WriteDirect(0x0200, 0x55);
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), id);

    // Evaluating again without memory change should not hit
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), 0U);
}

TEST_F(BreakpointExpressionTest, RegisterChangedExpression) {
    Breakpoint bp;
    BreakCondition cond;
    cond.type = BreakpointType::Expression;
    cond.expression = "X changed";
    bp.conditions.push_back(cond);

    uint32_t id = bpManager.AddBreakpoint(std::move(bp));
    ASSERT_GT(id, 0U);

    cpu.X = 0x10;
    // Baseline capture
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), 0U);

    // Same value
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), 0U);

    // Changed value
    cpu.X = 0x11;
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), id);

    // Same value again
    EXPECT_EQ(bpManager.Evaluate(cpu, bus), 0U);
}

