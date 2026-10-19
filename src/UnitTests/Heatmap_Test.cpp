#include <gtest/gtest.h>

#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

TEST(HeatmapTest, TrackingAndDecay) {
    Hardware::Bus bus;
    Hardware::RAM ram(0x10000);
    bus.RegisterDevice(0x0000, 0xFFFF, &ram, true, false);

    EXPECT_FALSE(bus.HasActiveHooks());
    EXPECT_FALSE(bus.IsHeatmapEnabled());

    bus.SetHeatmapEnabled(true);
    EXPECT_TRUE(bus.IsHeatmapEnabled());
    EXPECT_TRUE(bus.HasActiveHooks());

    bus.Write<true>(0x0150, 0x42);
    EXPECT_EQ(bus.GetWriteHeat(0x0150), 255);
    EXPECT_EQ(bus.GetReadHeat(0x0150), 0);

    EXPECT_EQ(bus.Read<true>(0x0020), 0);
    EXPECT_EQ(bus.GetReadHeat(0x0020), 255);
    EXPECT_EQ(bus.GetWriteHeat(0x0020), 0);

    EXPECT_EQ(bus.GetReadHeat(0x0000), 0);
    EXPECT_EQ(bus.GetWriteHeat(0x0000), 0);

    bus.DecayHeatmap(55);
    EXPECT_EQ(bus.GetWriteHeat(0x0150), 200);
    EXPECT_EQ(bus.GetReadHeat(0x0020), 200);

    bus.DecayHeatmap(250);
    EXPECT_EQ(bus.GetWriteHeat(0x0150), 0);
    EXPECT_EQ(bus.GetReadHeat(0x0020), 0);

    bus.Write<true>(0x0080, 0x11);
    EXPECT_EQ(bus.GetWriteHeat(0x0080), 255);

    bus.ClearHeatmap();
    EXPECT_EQ(bus.GetWriteHeat(0x0080), 0);

    bus.SetHeatmapEnabled(false);
    EXPECT_FALSE(bus.IsHeatmapEnabled());
    EXPECT_FALSE(bus.HasActiveHooks());
}

TEST(ProfilerTest, ReadWriteTrackingAndClear) {
    Hardware::Bus bus;
    Hardware::RAM ram(0x10000);
    bus.RegisterDevice(0x0000, 0xFFFF, &ram, true, false);

    bus.SetProfilingEnabled(true);

    bus.Write<true>(0x0200, 0xAA);
    bus.Write<true>(0x0200, 0xBB);
    EXPECT_EQ(bus.Read<true>(0x0200), 0xBB);
    EXPECT_EQ(bus.Read<true>(0x0300), 0x00);

    const uint32_t* counts = bus.GetProfilerCounts();
    const uint32_t* readCounts = bus.GetProfilerReadCounts();
    const uint32_t* writeCounts = bus.GetProfilerWriteCounts();

    EXPECT_EQ(counts[0x0200], 3);
    EXPECT_EQ(writeCounts[0x0200], 2);
    EXPECT_EQ(readCounts[0x0200], 1);

    EXPECT_EQ(counts[0x0300], 1);
    EXPECT_EQ(writeCounts[0x0300], 0);
    EXPECT_EQ(readCounts[0x0300], 1);

    bus.ClearProfiler();
    EXPECT_EQ(counts[0x0200], 0);
    EXPECT_EQ(writeCounts[0x0200], 0);
    EXPECT_EQ(readCounts[0x0200], 0);
    EXPECT_EQ(counts[0x0300], 0);
    EXPECT_EQ(writeCounts[0x0300], 0);
    EXPECT_EQ(readCounts[0x0300], 0);
}
