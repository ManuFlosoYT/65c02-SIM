#include "Hardware/Audio/SID.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>

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

