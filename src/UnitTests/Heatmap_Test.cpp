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

