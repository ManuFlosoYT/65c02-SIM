#include "Hardware/Comm/ACIA.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "Hardware/Core/Bus.h"
#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Core/Emulator/SerialIOBridge.h"

using namespace Hardware;

TEST(ACIA_Test, BaudRateDivisorCalculation) {
    ACIA acia;
    const int expectedBaud[16] = {
        0, 50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200
    };

    for (Byte code = 0; code < 16; ++code) {
        acia.Write(ACIA_CTRL, code);
        EXPECT_EQ(acia.GetBaudRate(), expectedBaud[code]);
        EXPECT_EQ(acia.ReadDirect(ACIA_CTRL) & 0x0F, code);
    }
}

TEST(ACIA_Test, FrameBitsCalculation) {
    ACIA acia;

    acia.Write(ACIA_CTRL, 0x00);
    acia.Write(ACIA_CMD, 0x00);
    EXPECT_EQ(acia.GetFrameBits(), 10);

    acia.Write(ACIA_CTRL, 0x20);
    EXPECT_EQ(acia.GetFrameBits(), 9);

    acia.Write(ACIA_CTRL, 0x40);
    EXPECT_EQ(acia.GetFrameBits(), 8);

    acia.Write(ACIA_CTRL, 0x60);
    EXPECT_EQ(acia.GetFrameBits(), 7);

    acia.Write(ACIA_CTRL, 0x00);
    acia.Write(ACIA_CMD, 0x20);
    EXPECT_EQ(acia.GetFrameBits(), 11);

    acia.Write(ACIA_CTRL, 0x80);
    acia.Write(ACIA_CMD, 0x00);
    EXPECT_EQ(acia.GetFrameBits(), 11);

    acia.Write(ACIA_CTRL, 0xA0);
    acia.Write(ACIA_CMD, 0x20);
    EXPECT_EQ(acia.GetFrameBits(), 11);
}

TEST(ACIA_Test, TxTransmission) {
    ACIA acia;
    acia.Write(ACIA_CMD, ACIA_CMD_DTR);
    acia.Write(ACIA_CTRL, 0x0F);

    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_TDRE, 0);

    std::string output;
    acia.SetOutputCallback([&output](char c) { output += c; });

    acia.Write(ACIA_DATA, 'Z');
    EXPECT_EQ(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_TDRE, 0);
    EXPECT_TRUE(output.empty());

    // ips = 192000, baud = 19200, frameBits = 10 -> total ticks = 100
    for (int i = 0; i < 101; ++i) {
        acia.Clock(192000);
    }
    EXPECT_EQ(output, "Z");
    EXPECT_TRUE(acia.IsIdle());
}

TEST(ACIA_Test, RxReceptionAndOverrun) {
    ACIA acia;

    acia.ReceiveData('A');
    EXPECT_EQ(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_RDRF, 0);

    acia.Write(ACIA_CMD, ACIA_CMD_DTR);
    EXPECT_TRUE(acia.CanReceive());

    acia.ReceiveData(0x42);
    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_RDRF, 0);
    EXPECT_EQ(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_OVRN, 0);
    EXPECT_FALSE(acia.CanReceive());
    EXPECT_EQ(acia.GetRxData(), 0x42);

    acia.ReceiveData(0x43);
    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_OVRN, 0);
    EXPECT_EQ(acia.GetRxData(), 0x43);

    Byte readVal = acia.Read(ACIA_DATA);
    EXPECT_EQ(readVal, 0x43);
    EXPECT_EQ(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_RDRF, 0);
    EXPECT_TRUE(acia.CanReceive());
    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_OVRN, 0);
}

TEST(ACIA_Test, InterruptGeneration) {
    ACIA acia;
    acia.Write(ACIA_CMD, ACIA_CMD_DTR);
    EXPECT_FALSE(acia.HasIRQ());

    acia.ReceiveData(0x55);
    EXPECT_TRUE(acia.HasIRQ());
    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_IRQ, 0);

    acia.Write(ACIA_CMD, ACIA_CMD_DTR | ACIA_CMD_RX_IRQ_DISABLE);
    EXPECT_FALSE(acia.HasIRQ());
    EXPECT_EQ(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_IRQ, 0);

    acia.Write(ACIA_CMD, ACIA_CMD_DTR);
    EXPECT_TRUE(acia.HasIRQ());

    acia.Read(ACIA_DATA);
    EXPECT_FALSE(acia.HasIRQ());
    EXPECT_EQ(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_IRQ, 0);

    acia.Write(ACIA_CMD, ACIA_CMD_DTR | 0x04);
    EXPECT_TRUE(acia.HasIRQ());
    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_IRQ, 0);

    acia.Write(ACIA_DATA, 'T');
    EXPECT_FALSE(acia.HasIRQ());
}

TEST(ACIA_Test, ProgrammedReset) {
    ACIA acia;
    acia.Write(ACIA_CMD, ACIA_CMD_DTR);
    acia.ReceiveData(0x11);
    acia.ReceiveData(0x22);

    EXPECT_TRUE(acia.HasIRQ());
    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_OVRN, 0);
    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_RDRF, 0);

    acia.Write(ACIA_STATUS, 0x00);
    EXPECT_FALSE(acia.HasIRQ());
    EXPECT_EQ(acia.ReadDirect(ACIA_STATUS) & (ACIA_STATUS_IRQ | ACIA_STATUS_OVRN | ACIA_STATUS_RDRF), 0);
    EXPECT_NE(acia.ReadDirect(ACIA_STATUS) & ACIA_STATUS_TDRE, 0);
}

TEST(ACIA_Test, Serialization) {
    ACIA acia1;
    acia1.Write(ACIA_CMD, ACIA_CMD_DTR | 0x04);
    acia1.Write(ACIA_CTRL, 0x2A);
    acia1.ReceiveData(0x7E);
    acia1.Write(ACIA_DATA, 0x3C);

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    EXPECT_TRUE(acia1.SaveState(ss));

    ACIA acia2;
    EXPECT_TRUE(acia2.LoadState(ss));

    EXPECT_EQ(acia2.ReadDirect(ACIA_CMD), acia1.ReadDirect(ACIA_CMD));
    EXPECT_EQ(acia2.ReadDirect(ACIA_CTRL), acia1.ReadDirect(ACIA_CTRL));
    EXPECT_EQ(acia2.ReadDirect(ACIA_STATUS), acia1.ReadDirect(ACIA_STATUS));
    EXPECT_EQ(acia2.GetRxData(), acia1.GetRxData());
    EXPECT_EQ(acia2.GetTxData(), acia1.GetTxData());
    EXPECT_EQ(acia2.GetBaudRate(), acia1.GetBaudRate());
    EXPECT_EQ(acia2.GetFrameBits(), acia1.GetFrameBits());
    EXPECT_EQ(acia2.HasIRQ(), acia1.HasIRQ());
}

TEST(ACIA_Test, SerialIOBridgeInjectTextBlockAndInterrupts) {
    Core::SerialIOBridge bridge;
    Hardware::ACIA acia;
    acia.Write(ACIA_CMD, ACIA_CMD_DTR);
    std::atomic<bool> pendingInterrupt{false};
    bridge.InjectText("A\r\nB\nC\rD");

    const Byte expectedChars[] = {'A', '\r', 'B', '\r', 'C', '\r', 'D'};
    for (Byte expected : expectedChars) {
        pendingInterrupt.store(false);
        bridge.ProcessSerialInput(acia, 1000000, pendingInterrupt);
        EXPECT_TRUE(acia.HasIRQ());
        EXPECT_TRUE(pendingInterrupt.load());
        EXPECT_EQ(acia.Read(ACIA_DATA), expected);
        EXPECT_FALSE(acia.HasIRQ());
        bridge.DecrementBaudDelay(bridge.GetBaudDelay());
    }
    EXPECT_FALSE(bridge.HasInput());
}

TEST(ACIA_Test, EmulatorWAIWokenByInjectedText) {
    Core::Emulator emulator;
    std::string error;
    emulator.InitFromMemory({}, "", error);

    emulator.GetROM().WriteDirect(0x7FFC, 0x00);
    emulator.GetROM().WriteDirect(0x7FFD, 0x10);
    emulator.GetROM().WriteDirect(0x7FFE, 0x00);
    emulator.GetROM().WriteDirect(0x7FFF, 0x20);

    emulator.GetMem().Write(0x1000, 0x58);
    emulator.GetMem().Write(0x1001, 0xCB);
    emulator.GetMem().Write(0x1002, 0xE8);
    emulator.GetMem().Write(0x1003, 0x4C);
    emulator.GetMem().Write(0x1004, 0x03);
    emulator.GetMem().Write(0x1005, 0x10);

    emulator.GetMem().Write(0x2000, 0xAD);
    emulator.GetMem().Write(0x2001, 0x00);
    emulator.GetMem().Write(0x2002, 0x50);
    emulator.GetMem().Write(0x2003, 0x40);

    emulator.Reset();
    emulator.GetACIA().Write(ACIA_CMD, ACIA_CMD_DTR);
    emulator.GetCPU().SetCycleAccurate(false);
    emulator.GetCPU().X = 0;

    emulator.Step<false>();
    emulator.Step<false>();
    EXPECT_TRUE(emulator.GetCPU().waiting);

    emulator.InjectText("Z");
    emulator.Step<false>();
    EXPECT_FALSE(emulator.GetCPU().waiting);
    EXPECT_EQ(emulator.GetCPU().PC, 0x2000);

    emulator.Step<false>();
    emulator.Step<false>();
    EXPECT_EQ(emulator.GetCPU().PC, 0x1002);

    emulator.Step<false>();
    EXPECT_EQ(emulator.GetCPU().X, 1);
}
