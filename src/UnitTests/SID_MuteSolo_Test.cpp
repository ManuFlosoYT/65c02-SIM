#include "Hardware/Audio/SID.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <vector>

using namespace Hardware;

TEST(SIDMuteSoloTest, DefaultStateIsActive) {
    SID sid;
    for (int i = 0; i < SID::MAX_SID_VOICES; ++i) {
        EXPECT_FALSE(sid.IsVoiceMuted(i));
        EXPECT_FALSE(sid.IsVoiceSolo(i));
    }
}

TEST(SIDMuteSoloTest, SetGetMuteAndSolo) {
    SID sid;

    sid.SetVoiceMute(0, true);
    EXPECT_TRUE(sid.IsVoiceMuted(0));
    EXPECT_FALSE(sid.IsVoiceMuted(1));
    EXPECT_FALSE(sid.IsVoiceMuted(2));

    sid.SetVoiceMute(0, false);
    EXPECT_FALSE(sid.IsVoiceMuted(0));

    sid.SetVoiceSolo(1, true);
    EXPECT_FALSE(sid.IsVoiceSolo(0));
    EXPECT_TRUE(sid.IsVoiceSolo(1));
    EXPECT_FALSE(sid.IsVoiceSolo(2));

    sid.SetVoiceSolo(1, false);
    EXPECT_FALSE(sid.IsVoiceSolo(1));

    // Out of bounds safety
    sid.SetVoiceMute(-1, true);
    sid.SetVoiceMute(5, true);
    sid.SetVoiceSolo(-1, true);
    sid.SetVoiceSolo(5, true);
    EXPECT_FALSE(sid.IsVoiceMuted(-1));
    EXPECT_FALSE(sid.IsVoiceMuted(5));
    EXPECT_FALSE(sid.IsVoiceSolo(-1));
    EXPECT_FALSE(sid.IsVoiceSolo(5));
}

TEST(SIDMuteSoloTest, AudioSnapshotBuffer) {
    SID sid;
    std::array<float, 64> buffer{};
    buffer.fill(123.0F);

    sid.GetAudioSnapshot(buffer);
    for (float sample : buffer) {
        EXPECT_EQ(sample, 0.0F);
    }
}

TEST(SIDTrackerTest, CaptureWritesAndFrames) {
    SID sid;

    EXPECT_EQ(sid.GetRegister(0), 0);
    EXPECT_EQ(sid.GetRegisters().at(0), 0);

    sid.Write(0x00, 0x42);
    EXPECT_EQ(sid.GetRegister(0x00), 0x42);
    EXPECT_EQ(sid.GetRegisters().at(0x00), 0x42);

    std::vector<SIDTrackerFrame> log;
    sid.GetTrackerLog(log);
    ASSERT_EQ(log.size(), 1U);
    EXPECT_EQ(log[0].registers.at(0), 0x42);
    EXPECT_EQ(log[0].writeMask, 1U << 0);
    EXPECT_EQ(log[0].frameNumber, 0U);

    sid.ClearTrackerLog();
    sid.GetTrackerLog(log);
    EXPECT_TRUE(log.empty());

    sid.SetEmulationPaused(false);
    sid.Write(0x01, 0x10);
    sid.Write(0x04, 0x11);

    sid.Clock(20000);
    sid.GetTrackerLog(log);
    ASSERT_FALSE(log.empty());
    EXPECT_EQ(log[0].registers.at(0x01), 0x10);
    EXPECT_EQ(log[0].registers.at(0x04), 0x11);
    EXPECT_NE(log[0].writeMask & (1U << 0x01), 0U);
    EXPECT_NE(log[0].writeMask & (1U << 0x04), 0U);

    sid.SetEmulationPaused(true);
    for (int i = 0; i < 600; ++i) {
        sid.Write(0x00, static_cast<uint8_t>(i & 0xFF));
    }
    sid.GetTrackerLog(log);
    EXPECT_EQ(log.size(), 512U);

    sid.Reset();
    sid.GetTrackerLog(log);
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(sid.GetRegister(0x01), 0);
}

