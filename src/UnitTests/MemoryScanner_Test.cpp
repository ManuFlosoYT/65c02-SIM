#include "Frontend/Control/MemoryScanner.h"

#include <gtest/gtest.h>

#include <array>

using namespace Control;

struct ScannerFixture : ::testing::Test {
    std::array<std::uint8_t, 0x10000> mem{};
    auto Reader() {
        return [this](std::uint16_t addr) { return mem.at(addr); };
    }
};

TEST_F(ScannerFixture, U8ExactThenFilters) {
    mem.at(0x10) = 5;
    mem.at(0x20) = 5;
    mem.at(0x30) = 7;
    ScanQuery query{.type = ScanType::U8, .value = 5};
    auto cands = FirstScan(query, 0, 0xFF, false, Reader());
    ASSERT_EQ(cands.size(), 2U);

    mem.at(0x10) = 6;
    query.filter = ScanFilter::Increased;
    NextScan(cands, query, Reader());
    ASSERT_EQ(cands.size(), 1U);
    EXPECT_EQ(cands[0].address, 0x10);
    EXPECT_EQ(cands[0].previous, 6);

    query.filter = ScanFilter::Unchanged;
    NextScan(cands, query, Reader());
    EXPECT_EQ(cands.size(), 1U);

    mem.at(0x10) = 1;
    query.filter = ScanFilter::Decreased;
    NextScan(cands, query, Reader());
    EXPECT_EQ(cands.size(), 1U);

    query.filter = ScanFilter::Exact;
    query.value = 9;
    NextScan(cands, query, Reader());
    EXPECT_TRUE(cands.empty());
}

TEST_F(ScannerFixture, UnknownSnapshotCoversRange) {
    ScanQuery query{.type = ScanType::U8};
    EXPECT_EQ(FirstScan(query, 0x10, 0x1F, true, Reader()).size(), 16U);
}

TEST_F(ScannerFixture, U16LittleEndianCarryAndRangeEdge) {
    mem.at(0x40) = 0xFF;
    mem.at(0x41) = 0x00;
    ScanQuery query{.type = ScanType::U16, .value = 0x00FF};
    auto cands = FirstScan(query, 0x40, 0x41, false, Reader());
    ASSERT_EQ(cands.size(), 1U);  // 0x41 does not fit a full word in range

    mem.at(0x40) = 0x00;
    mem.at(0x41) = 0x01;
    query.filter = ScanFilter::Increased;
    NextScan(cands, query, Reader());
    ASSERT_EQ(cands.size(), 1U);
    EXPECT_EQ(cands[0].previous, 0x0100);
}

TEST_F(ScannerFixture, StringFindsAndRefilters) {
    const std::string text = "HI";
    for (std::size_t i = 0; i < 2; ++i) {
        mem.at(0x50 + i) = static_cast<std::uint8_t>(text[i]);
        mem.at(0x60 + i) = static_cast<std::uint8_t>(text[i]);
    }
    ScanQuery query{.type = ScanType::String, .text = "HI"};
    auto cands = FirstScan(query, 0, 0xFF, false, Reader());
    ASSERT_EQ(cands.size(), 2U);

    mem.at(0x60) = 'X';
    NextScan(cands, query, Reader());
    ASSERT_EQ(cands.size(), 1U);
    EXPECT_EQ(cands[0].address, 0x50);

    EXPECT_TRUE(FirstScan(query, 0, 0x50, false, Reader()).empty());  // text cut by range end
}

