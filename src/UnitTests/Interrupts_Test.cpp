#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Memory/RAM.h"

using namespace Core;
using namespace Hardware;

TEST(InterruptsTest, NMIPriorityOverIRQ) {
    Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);

    emulator.GetROM().WriteDirect(0x7FFC, 0x00);
    emulator.GetROM().WriteDirect(0x7FFD, 0x10);
    emulator.GetROM().WriteDirect(0x7FFA, 0x00);
    emulator.GetROM().WriteDirect(0x7FFB, 0x20);
    emulator.GetROM().WriteDirect(0x7FFE, 0x00);
    emulator.GetROM().WriteDirect(0x7FFF, 0x30);

    emulator.GetMem().Write(0x1000, 0xEA);
    emulator.GetMem().Write(0x2000, 0x40);
    emulator.GetMem().Write(0x3000, 0x40);

    emulator.Reset();
    auto& cpu = emulator.GetCPU();
    cpu.SetCycleAccurate(false);
    cpu.I = 0;

    emulator.TriggerIRQ();
    emulator.TriggerNMI();

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x2000);
    EXPECT_NE(cpu.PC, 0x3000);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x3000);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x1001);
}

TEST(InterruptsTest, NMIEdgeDetectionDoesNotRetrigger) {
    Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);

    emulator.GetROM().WriteDirect(0x7FFC, 0x00);
    emulator.GetROM().WriteDirect(0x7FFD, 0x10);
    emulator.GetROM().WriteDirect(0x7FFA, 0x00);
    emulator.GetROM().WriteDirect(0x7FFB, 0x20);

    emulator.GetMem().Write(0x1000, 0xEA);
    emulator.GetMem().Write(0x1001, 0xE8);
    emulator.GetMem().Write(0x1002, 0xEA);
    emulator.GetMem().Write(0x2000, 0x40);

    emulator.Reset();
    auto& cpu = emulator.GetCPU();
    cpu.SetCycleAccurate(false);
    cpu.X = 0;

    emulator.TriggerNMI();

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x2000);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x1001);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x1002);
    EXPECT_EQ(cpu.X, 1);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x1003);

    emulator.TriggerNMI();
    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x2000);
}

TEST(InterruptsTest, IRQLevelDetectionRetriggersUntilDeviceCleared) {
    Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);

    emulator.GetROM().WriteDirect(0x7FFC, 0x00);
    emulator.GetROM().WriteDirect(0x7FFD, 0x10);
    emulator.GetROM().WriteDirect(0x7FFE, 0x00);
    emulator.GetROM().WriteDirect(0x7FFF, 0x20);

    emulator.GetMem().Write(0x1000, 0xEA);
    emulator.GetMem().Write(0x1001, 0xE8);
    emulator.GetMem().Write(0x2000, 0x40);

    emulator.Reset();
    auto& cpu = emulator.GetCPU();
    cpu.SetCycleAccurate(false);
    cpu.I = 0;

    emulator.GetMem().Write(IER, 0x80 | 0x40);
    emulator.GetMem().Write(ACR, 0x00);
    emulator.GetMem().Write(T1C_L, 0x01);
    emulator.GetMem().Write(T1C_H, 0x00);
    emulator.GetVIA().Clock();
    emulator.GetVIA().Clock();
    EXPECT_TRUE(emulator.GetVIA().isIRQAsserted());

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x2000);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x2000);

    emulator.GetMem().Read(T1C_L);
    EXPECT_FALSE(emulator.GetVIA().isIRQAsserted());

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x1001);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x1002);
    EXPECT_EQ(cpu.X, 1);
}

TEST(InterruptsTest, WAIBehaviorWhenInterruptDisabled) {
    Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);

    emulator.GetROM().WriteDirect(0x7FFC, 0x00);
    emulator.GetROM().WriteDirect(0x7FFD, 0x10);
    emulator.GetROM().WriteDirect(0x7FFE, 0x00);
    emulator.GetROM().WriteDirect(0x7FFF, 0x30);

    emulator.GetMem().Write(0x1000, 0x78);
    emulator.GetMem().Write(0x1001, 0xCB);
    emulator.GetMem().Write(0x1002, 0xE8);
    emulator.GetMem().Write(0x1003, 0xEA);
    emulator.GetMem().Write(0x3000, 0x40);

    emulator.Reset();
    auto& cpu = emulator.GetCPU();
    cpu.SetCycleAccurate(false);
    cpu.X = 0;

    emulator.Step<false>();
    EXPECT_TRUE(cpu.I);
    EXPECT_EQ(cpu.PC, 0x1001);

    emulator.Step<false>();
    EXPECT_TRUE(cpu.waiting);
    EXPECT_EQ(cpu.PC, 0x1002);
    EXPECT_EQ(cpu.X, 0);

    emulator.Step<false>();
    EXPECT_TRUE(cpu.waiting);
    EXPECT_EQ(cpu.PC, 0x1002);
    EXPECT_EQ(cpu.X, 0);

    emulator.TriggerIRQ();

    emulator.Step<false>();
    EXPECT_FALSE(cpu.waiting);
    EXPECT_NE(cpu.PC, 0x3000);
    EXPECT_EQ(cpu.PC, 0x1002);

    emulator.Step<false>();
    EXPECT_EQ(cpu.X, 1);
    EXPECT_EQ(cpu.PC, 0x1003);
    EXPECT_NE(cpu.PC, 0x3000);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x1004);
    EXPECT_EQ(cpu.X, 1);
}

TEST(InterruptsTest, BRKHijackedByNMI_Emulator) {
    Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);

    emulator.GetROM().WriteDirect(0x7FFC, 0x00);
    emulator.GetROM().WriteDirect(0x7FFD, 0x10);
    emulator.GetROM().WriteDirect(0x7FFA, 0x00);
    emulator.GetROM().WriteDirect(0x7FFB, 0x20);
    emulator.GetROM().WriteDirect(0x7FFE, 0x00);
    emulator.GetROM().WriteDirect(0x7FFF, 0x30);

    emulator.GetMem().Write(0x1000, 0x00);
    emulator.GetMem().Write(0x1001, 0x00);
    emulator.GetMem().Write(0x1002, 0xEA);
    emulator.GetMem().Write(0x2000, 0x40);
    emulator.GetMem().Write(0x3000, 0x40);

    emulator.Reset();
    auto& cpu = emulator.GetCPU();
    cpu.SetCycleAccurate(false);

    emulator.TriggerNMI();

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x2000);
    EXPECT_NE(cpu.PC, 0x3000);

    EXPECT_EQ(cpu.SP, 0x01FC);
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x01FF), 0x10);
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x01FE), 0x02);
    Byte pushedStatus = emulator.GetMem().ReadDirect(0x01FD);
    EXPECT_TRUE((pushedStatus & 0x10) != 0);
}

TEST(InterruptsTest, BRKStandaloneWithoutNMI_TakesIRQVector) {
    Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);

    emulator.GetROM().WriteDirect(0x7FFC, 0x00);
    emulator.GetROM().WriteDirect(0x7FFD, 0x10);
    emulator.GetROM().WriteDirect(0x7FFA, 0x00);
    emulator.GetROM().WriteDirect(0x7FFB, 0x20);
    emulator.GetROM().WriteDirect(0x7FFE, 0x00);
    emulator.GetROM().WriteDirect(0x7FFF, 0x30);

    emulator.GetMem().Write(0x1000, 0x00);
    emulator.GetMem().Write(0x1001, 0x00);
    emulator.GetMem().Write(0x3000, 0x40);

    emulator.Reset();
    auto& cpu = emulator.GetCPU();
    cpu.SetCycleAccurate(false);

    emulator.Step<false>();
    EXPECT_EQ(cpu.PC, 0x3000);

    EXPECT_EQ(cpu.SP, 0x01FC);
    Byte pushedStatus = emulator.GetMem().ReadDirect(0x01FD);
    EXPECT_TRUE((pushedStatus & 0x10) != 0);
}

TEST(InterruptsTest, CPU_BRKHijackedByNMI_Direct) {
    Bus bus;
    RAM ram{0x10000};
    bus.RegisterDevice(0x0000, 0xFFFF, &ram);

    CPU cpu;
    cpu.Reset();

    bus.WriteDirect(0xFFFA, 0x00);
    bus.WriteDirect(0xFFFB, 0x20);
    bus.WriteDirect(0xFFFE, 0x00);
    bus.WriteDirect(0xFFFF, 0x30);

    bus.Write(0x1000, INS_BRK);
    bus.Write(0x1001, 0x00);
    bus.Write(0x2000, INS_STP);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);

    cpu.TriggerNMI();
    EXPECT_TRUE(cpu.HasPendingNMI());

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x2001);
    EXPECT_FALSE(cpu.HasPendingNMI());

    EXPECT_EQ(bus.ReadDirect(0x01FF), 0x10);
    EXPECT_EQ(bus.ReadDirect(0x01FE), 0x02);
    Byte pushedStatus = bus.ReadDirect(0x01FD);
    EXPECT_TRUE((pushedStatus & 0x10) != 0);
}

