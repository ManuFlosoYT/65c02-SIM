#include <gtest/gtest.h>

#include "Hardware/Comm/ACIA.h"
#include "Hardware/Comm/SDCard.h"
#include "Hardware/Comm/VIA.h"
#include "Hardware/Core/Emulator.h"
#include "Hardware/Core/LogicAnalyzer.h"

TEST(LogicAnalyzerTest, RingBufferOperations) {
    Hardware::LogicAnalyzer analyzer;
    EXPECT_EQ(analyzer.GetTotalSamples(), 0);
    EXPECT_EQ(analyzer.GetCount(), 0);
    EXPECT_FALSE(analyzer.IsFrozen());

    Hardware::LogicSample emptySample = analyzer.GetSample(0);
    EXPECT_EQ(emptySample.cycle, 0);

    for (uint64_t i = 0; i < 10; ++i) {
        Hardware::LogicSample sample{};
        sample.cycle = i;
        sample.address = static_cast<uint16_t>(0x1000 + i);
        sample.data = static_cast<uint8_t>(i);
        analyzer.RecordSample(sample);
    }

    EXPECT_EQ(analyzer.GetTotalSamples(), 10);
    EXPECT_EQ(analyzer.GetCount(), 10);

    EXPECT_EQ(analyzer.GetSample(0).cycle, 9);
    EXPECT_EQ(analyzer.GetSample(0).address, 0x1009);
    EXPECT_EQ(analyzer.GetSample(1).cycle, 8);
    EXPECT_EQ(analyzer.GetSample(9).cycle, 0);

    EXPECT_EQ(analyzer.GetSampleByIndex(0).cycle, 0);
    EXPECT_EQ(analyzer.GetSampleByIndex(9).cycle, 9);
}

TEST(LogicAnalyzerTest, BufferWrapAround) {
    Hardware::LogicAnalyzer analyzer;
    constexpr size_t TOTAL = Hardware::LogicAnalyzer::BUFFER_SIZE + 500;

    for (size_t i = 0; i < TOTAL; ++i) {
        Hardware::LogicSample sample{};
        sample.cycle = i;
        sample.address = static_cast<uint16_t>(i & 0xFFFF);
        analyzer.RecordSample(sample);
    }

    EXPECT_EQ(analyzer.GetTotalSamples(), TOTAL);
    EXPECT_EQ(analyzer.GetCount(), Hardware::LogicAnalyzer::BUFFER_SIZE);

    EXPECT_EQ(analyzer.GetSample(0).cycle, TOTAL - 1);
    EXPECT_EQ(analyzer.GetSample(Hardware::LogicAnalyzer::BUFFER_SIZE - 1).cycle, TOTAL - Hardware::LogicAnalyzer::BUFFER_SIZE);

    // Clamps to oldest available
    EXPECT_EQ(analyzer.GetSample(TOTAL + 100).cycle, TOTAL - Hardware::LogicAnalyzer::BUFFER_SIZE);
}

TEST(LogicAnalyzerTest, FreezeAndClear) {
    Hardware::LogicAnalyzer analyzer;

    Hardware::LogicSample s1{};
    s1.cycle = 100;
    analyzer.RecordSample(s1);
    EXPECT_EQ(analyzer.GetTotalSamples(), 1);

    analyzer.SetFrozen(true);
    EXPECT_TRUE(analyzer.IsFrozen());

    Hardware::LogicSample s2{};
    s2.cycle = 200;
    analyzer.RecordSample(s2);
    EXPECT_EQ(analyzer.GetTotalSamples(), 1);
    EXPECT_EQ(analyzer.GetSample(0).cycle, 100);

    analyzer.SetFrozen(false);
    EXPECT_FALSE(analyzer.IsFrozen());

    analyzer.RecordSample(s2);
    EXPECT_EQ(analyzer.GetTotalSamples(), 2);
    EXPECT_EQ(analyzer.GetSample(0).cycle, 200);

    analyzer.Clear();
    EXPECT_EQ(analyzer.GetTotalSamples(), 0);
    EXPECT_EQ(analyzer.GetCount(), 0);

    analyzer.SetFrozen(true);
    analyzer.Reset();
    EXPECT_FALSE(analyzer.IsFrozen());
    EXPECT_EQ(analyzer.GetTotalSamples(), 0);
}

TEST(LogicAnalyzerTest, PinGetters) {
    Hardware::VIA via;
    via.Reset();
    EXPECT_TRUE(via.GetCA1());
    EXPECT_TRUE(via.GetCA2());
    EXPECT_TRUE(via.GetCB1());
    EXPECT_TRUE(via.GetCB2());

    via.SetCA1(false);
    EXPECT_FALSE(via.GetCA1());
    via.SetCA1(true);
    EXPECT_TRUE(via.GetCA1());

    Hardware::ACIA acia;
    acia.Reset();
    EXPECT_TRUE(acia.GetTxPin());
    EXPECT_TRUE(acia.GetRxPin());

    Hardware::SDCard sdcard;
    sdcard.Reset();
    EXPECT_FALSE(sdcard.IsCSActive());
    EXPECT_EQ(sdcard.GetLastMISO(), 0xFF);

    sdcard.SetCS(false);  // active low: false means CS asserted
    EXPECT_TRUE(sdcard.IsCSActive());
    sdcard.SetCS(true);   // true means CS deasserted
    EXPECT_FALSE(sdcard.IsCSActive());
}

TEST(LogicAnalyzerTest, EmulatorIntegration) {
    Core::Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);

    EXPECT_FALSE(emulator.IsLogicAnalyzerEnabled());
    emulator.SetLogicAnalyzerEnabled(true);
    EXPECT_TRUE(emulator.IsLogicAnalyzerEnabled());

    emulator.SampleLogicAnalyzer(0x8000, 0xEA, true, true);
    EXPECT_EQ(emulator.GetLogicAnalyzer().GetTotalSamples(), 1);

    Hardware::LogicSample s = emulator.GetLogicAnalyzer().GetSample(0);
    EXPECT_EQ(s.address, 0x8000);
    EXPECT_EQ(s.data, 0xEA);
    EXPECT_EQ(s.rw, 1);
    EXPECT_EQ(s.sync, 1);
    EXPECT_EQ(s.spiCs, 1);
    EXPECT_EQ(s.uartTx, 1);
    EXPECT_EQ(s.uartRx, 1);

    // Setup a NOP at reset vector and step
    emulator.GetMem().WriteDirect(0x8000, 0xEA);  // NOP
    emulator.GetMem().WriteDirect(0xFFFC, 0x00);
    emulator.GetMem().WriteDirect(0xFFFD, 0x80);
    emulator.Reset();
    emulator.SetLogicAnalyzerEnabled(true);
    emulator.Step();

    EXPECT_GE(emulator.GetLogicAnalyzer().GetTotalSamples(), 2);
}
