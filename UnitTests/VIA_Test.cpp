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
        bus.RegisterDevice(0x0000, 0xFFFF, &ram, true, false);  // RAM base
        bus.RegisterDevice(0x6000, 0x600F, &via, true, true);   // VIA on top
        via.Reset();
    }
};

TEST_F(VIA_Test, RegisterReadWrite) {
    // Test DDRB
    bus.Write(DDRB, 0xFF);  // Set all pins to Output
    EXPECT_EQ(via.GetDDRB(), 0xFF);
    EXPECT_EQ(bus.Read(DDRB), 0xFF);

    // Test ORB/IRB (Now in Output Mode)
    bus.Write(PORTB, 0x55);
    // Read should return Output Register because DDR is 1
    // Read = (Input & ~DDR) | (Output & DDR) = (Input & 0) | (0x55 & 0xFF) =
    // 0x55
    EXPECT_EQ(via.GetPortB(), 0x55);
    EXPECT_EQ(bus.Read(PORTB), 0x55);

    // Test Input Pins (IRA/IRB) when DDR is 0
    // Set DDRB to 0xF0 (Upper Output, Lower Input)
    bus.Write(DDRB, 0xF0);
    // Set InputB (External) to 0x05 (0000 0101)
    via.SetInputB(0x05);
    // Set ORA to 0xAA (1010 1010)
    bus.Write(PORTB, 0xAA);

    // Read PORTB
    // Bits 7-4 (Output): ORB & DDRB = 0xAA & 0xF0 = 0xA0
    // Bits 3-0 (Input):  IRB & ~DDRB = 0x05 & 0x0F = 0x05
    // Result: 0xA5
    EXPECT_EQ(via.GetPortB(), 0xA5);
    EXPECT_EQ(bus.Read(PORTB), 0xA5);
}

TEST_F(VIA_Test, ShiftRegister_ShiftIn_Phi2) {
    // Mode 2: Shift In under PHI2 control
    // Bit 4,3,2 = 010 -> 2
    bus.Write(ACR, 0x08);

    // Write CA1/CB1 logic if needed? No, PHI2 is internal clock.
    // Spec says: Shift In under PHI2.
    // CB2 (val) -> SR[0], SR << 1

    // Set CB2 input to 1
    via.SetCB2(true);

    // Trigger SR by reading/writing it?
    // Spec: "Writing or reading the Shift Register starts the shifting
    // process."
    bus.Read(SR);  // Start shifting

    // Clock 8 times. Each clock should shift in '1'.
    for (int i = 0; i < 8; i++) via.Clock();

    // Interrupt should be set
    EXPECT_TRUE(bus.Read(IFR) & 0x04);
    // SR should be 0xFF (shifted in 8 ones) if started from 0
    EXPECT_EQ(bus.Read(SR), 0xFF);
}

TEST_F(VIA_Test, PulseCounting_T2) {
    // Mode: Pulse Counting (ACR bit 5 = 1)
    bus.Write(ACR, 0x20);

    // Write T2C_L (Latch Low)
    bus.Write(T2C_L, 0x02);
    // Write T2C_H (Counter High - Starts T2)
    bus.Write(T2C_H, 0x00);  // CNT = 2

    EXPECT_FALSE(via.isIRQAsserted());

    // Clocking VIA normally shouldn't decrement T2 in this mode
    via.Clock();
    via.Clock();
    // T2 should still be 2 (or slightly delayed but not counting down freely)
    // Actually our implementation only decrements on SetInputB edges.

    // Simulate PB6 Pulses
    // High -> Low transition decrements
    via.SetInputB(0x40);  // High
    via.SetInputB(0x00);  // Low (Decrement 2->1)

    via.SetInputB(0x40);  // High
    via.SetInputB(0x00);  // Low (Decrement 1->0)

    via.SetInputB(0x40);  // High
    via.SetInputB(0x00);  // Low (Decrement 0->-1, Interrupt)

    EXPECT_TRUE(bus.Read(IFR) & 0x20);  // T2 Interrupt
}

TEST_F(VIA_Test, Timer1_OneShot) {
    // 1. Write T1C_L (Latch Low)
    bus.Write(T1C_L, 0x10);
    EXPECT_EQ(via.GetT1L_L(), 0x10);

    // 2. Write T1C_H (Counter High - starts timer)
    bus.Write(T1C_H, 0x00);  // CNT = 0x0010 (16 decimal)

    // Check if IFR flag is cleared
    EXPECT_FALSE(via.isIRQAsserted());

    // Clock 16 times
    for (int i = 0; i < 16; i++) {
        via.Clock();
        EXPECT_FALSE(via.isIRQAsserted())
            << "IRQ asserted prematurely at cycle " << i;
    }

    // 17th clock should trigger Underflow (0 -> -1) and set Interrupt
    via.Clock();
    // Check IFR bit 6 (Timer 1)
    EXPECT_EQ(bus.Read(IFR) & 0x40, 0x40);
}

TEST_F(VIA_Test, InterruptEnable) {
    // Enable Timer 1 Interrupt in IER (Set bit 7 + bit 6)
    bus.Write(IER, 0x80 | 0x40);
    EXPECT_EQ(bus.Read(IER) & 0x40, 0x40);

    // Trigger T1 Interrupt
    bus.Write(ACR, 0x00);  // One shot
    bus.Write(T1C_L, 0x01);
    bus.Write(T1C_H, 0x00);  // 1 cycle

    via.Clock();  // 0
    via.Clock();  // -1 (Interrupt)

    EXPECT_TRUE(via.isIRQAsserted());

    // Disable T1 Interrupt (Clear bit 7 + bit 6)
    bus.Write(IER, 0x00 | 0x40);  // 0x00 in bit 7 means clear
    EXPECT_EQ(bus.Read(IER) & 0x40, 0x00);

    // IRQ line should now be high (not asserted) because IER cached it out
    EXPECT_FALSE(via.isIRQAsserted());
}
