#include "Hardware/Comm/VIA.h"

#include <gtest/gtest.h>

#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class VIA_Test : public ::testing::Test {
protected:
    Bus bus;
    RAM ram{0x10000};
    VIA via;

    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram, true, false);
        bus.RegisterDevice(0x6000, 0x600F, &via, true, true);
        via.Reset();
    }
};

TEST_F(VIA_Test, RegisterReadWrite) {
    bus.Write(DDRB, 0xFF);
    EXPECT_EQ(via.GetDDRB(), 0xFF);
    EXPECT_EQ(bus.Read(DDRB), 0xFF);

    bus.Write(PORTB, 0x55);
    EXPECT_EQ(via.GetPortB(), 0x55);
    EXPECT_EQ(bus.Read(PORTB), 0x55);

    bus.Write(DDRB, 0xF0);
    via.SetInputB(0x05);
    bus.Write(PORTB, 0xAA);

    EXPECT_EQ(via.GetPortB(), 0xA5);
    EXPECT_EQ(bus.Read(PORTB), 0xA5);
}

TEST_F(VIA_Test, ShiftRegister_ShiftIn_Phi2) {
    bus.Write(ACR, 0x08);

    // W65C22 Mode 2: Shift in on PHI2 clock

    via.SetCB2(true);

    bus.Read(VIA_SR);

    for (int i = 0; i < 8; i++) via.Clock();

    EXPECT_TRUE(bus.Read(IFR) & 0x04);
    EXPECT_EQ(bus.Read(VIA_SR), 0xFF);
}

TEST_F(VIA_Test, PulseCounting_T2) {
    bus.Write(ACR, 0x20);

    bus.Write(T2C_L, 0x02);
    bus.Write(T2C_H, 0x00);

    EXPECT_FALSE(via.isIRQAsserted());

    via.Clock();
    via.Clock();

    via.SetInputB(0x40);
    via.SetInputB(0x00);

    via.SetInputB(0x40);
    via.SetInputB(0x00);

    via.SetInputB(0x40);
    via.SetInputB(0x00);

    EXPECT_TRUE(bus.Read(IFR) & 0x20);
}

TEST_F(VIA_Test, Timer1_OneShot) {
    bus.Write(T1C_L, 0x10);
    EXPECT_EQ(via.GetT1L_L(), 0x10);

    bus.Write(T1C_H, 0x00);

    EXPECT_FALSE(via.isIRQAsserted());

    for (int i = 0; i < 16; i++) {
        via.Clock();
        EXPECT_FALSE(via.isIRQAsserted())
            << "IRQ asserted prematurely at cycle " << i;
    }

    via.Clock();
    EXPECT_EQ(bus.Read(IFR) & 0x40, 0x40);
}

TEST_F(VIA_Test, InterruptEnable) {
    bus.Write(IER, 0x80 | 0x40);
    EXPECT_EQ(bus.Read(IER) & 0x40, 0x40);

    bus.Write(ACR, 0x00);
    bus.Write(T1C_L, 0x01);
    bus.Write(T1C_H, 0x00);

    via.Clock();
    via.Clock();

    EXPECT_TRUE(via.isIRQAsserted());

    bus.Write(IER, 0x00 | 0x40);
    EXPECT_EQ(bus.Read(IER) & 0x40, 0x00);

    EXPECT_FALSE(via.isIRQAsserted());
}
