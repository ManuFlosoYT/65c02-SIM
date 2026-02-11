#include "../Hardware/LCD.h"

#include <gtest/gtest.h>

#include <iostream>

#include "../Hardware/Mem.h"

class TestLCD : public ::testing::Test {
protected:
    Mem mem;
    LCD lcd;

    void SetUp() override {
        mem.Init();
        lcd.Inicializar(mem);
        lcd.SetOutputCallback([](char c) {
            std::cout << c;
            std::cout.flush();
        });
    }

    // Helper to simulate VIA 6522 writes to PORTB
    void simulate_write(Byte val) { mem.Write(PORTB, val); }
};

TEST_F(TestLCD, PrintHello) {
    testing::internal::CaptureStdout();

    // 1. Initialization Sequence
    // Simulating LCDINIT

    // Send 0x03 (Function Set 8-bit) - 3 times
    // My implementation ignores these as it waits for 0x02 to switch to 4-bit
    // mode

    // 1st 0x03
    simulate_write(0x03);
    simulate_write(0x43);  // E=1
    simulate_write(0x03);  // E=0

    // 2nd 0x03
    simulate_write(0x03);
    simulate_write(0x43);
    simulate_write(0x03);

    // 3rd 0x03
    simulate_write(0x03);
    simulate_write(0x43);
    simulate_write(0x03);

    // Switch to 4-bit mode (Command 0x02)
    // RS=0, RW=0. Data=0x02.
    simulate_write(0x02);
    simulate_write(0x42);  // E=1
    simulate_write(0x02);  // E=0 -> Trigger: Mode 4-bit

    // Now in 4-bit mode.
    // Send Function Set (0x28) => 4-bit, 2-line, 5x8.
    // High Nibble: 0x2
    simulate_write(0x02);
    simulate_write(0x42);
    simulate_write(0x02);
    // Low Nibble: 0x8
    simulate_write(0x08);
    simulate_write(0x48);
    simulate_write(0x08);
    // Command executed (Ignored by print logic)

    // Send 'H' (0x48)
    // High Nibble: 0x4 (0100). RS=1 (0x10). -> 0x14
    simulate_write(0x14);
    simulate_write(0x54);
    simulate_write(0x14);
    // Low Nibble: 0x8 (1000). RS=1. -> 0x18
    simulate_write(0x18);
    simulate_write(0x58);
    simulate_write(0x18);
    // 'H' printed

    // Send 'e' (0x65)
    // High: 0x6. RS=1. -> 0x16
    simulate_write(0x16);
    simulate_write(0x56);
    simulate_write(0x16);
    // Low: 0x5. RS=1. -> 0x15
    simulate_write(0x15);
    simulate_write(0x55);
    simulate_write(0x15);

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "He");
}
