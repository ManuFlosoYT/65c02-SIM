#include "Hardware/Video/LCD.h"

#include <gtest/gtest.h>

#include <iostream>

#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class TestLCD : public ::testing::Test {
protected:
    Bus bus;
    RAM ram{0x10000};
    LCD lcd;

    void SetUp() override {
        bus.Init();
        lcd.Reset();
        lcd.SetOutputCallback([](char c) {
            std::cout << c;
            std::cout.flush();
        });
    }

    void simulate_write(Byte val) { lcd.Update(val); }
};

TEST_F(TestLCD, PrintHello) {
    testing::internal::CaptureStdout();



    simulate_write(0x03);
    simulate_write(0x43);
    simulate_write(0x03);

    simulate_write(0x03);
    simulate_write(0x43);
    simulate_write(0x03);

    simulate_write(0x03);
    simulate_write(0x43);
    simulate_write(0x03);

    simulate_write(0x02);
    simulate_write(0x42);
    simulate_write(0x02);

    simulate_write(0x02);
    simulate_write(0x42);
    simulate_write(0x02);
    simulate_write(0x08);
    simulate_write(0x48);
    simulate_write(0x08);

    simulate_write(0x14);
    simulate_write(0x54);
    simulate_write(0x14);
    simulate_write(0x18);
    simulate_write(0x58);
    simulate_write(0x18);

    simulate_write(0x16);
    simulate_write(0x56);
    simulate_write(0x16);
    simulate_write(0x15);
    simulate_write(0x55);
    simulate_write(0x15);

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "He");
}
