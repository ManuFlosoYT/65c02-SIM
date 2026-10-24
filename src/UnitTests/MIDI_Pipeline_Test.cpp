#include <gtest/gtest.h>

#include "Frontend/GUI/MIDI/MIDIFile.h"
#include "Frontend/GUI/MIDI/MIDISoundBank.h"

namespace {

using namespace GUI::MIDI;

TEST(MIDISoundBankTest, MelodicPresetsLookup) {
    MIDISoundBank::EnsureLoaded();
    EXPECT_FALSE(MIDISoundBank::GetMelodicPresets().empty());

    const SIDPatchParams piano = MIDISoundBank::GetInstrumentParams(0, 0, 60, 100);
    EXPECT_EQ(piano.name, "Piano");
    EXPECT_EQ(piano.waveMask, SID_WAVE_PULSE);
    EXPECT_EQ(piano.attackDecay, 0x09);
    EXPECT_EQ(piano.pulseWidth, 0x0600);

    const SIDPatchParams overdriveGuitar = MIDISoundBank::GetInstrumentParams(30, 0, 64, 110);
    EXPECT_EQ(overdriveGuitar.name, "Electric/Overdrive Guitar");
    EXPECT_EQ(overdriveGuitar.waveMask, static_cast<std::uint8_t>(SID_WAVE_SAW | SID_WAVE_PULSE));
    EXPECT_NE(overdriveGuitar.features & static_cast<std::uint16_t>(SIDFeatureFlags::HardSync), 0U);

    const SIDPatchParams squareLead = MIDISoundBank::GetInstrumentParams(80, 0, 72, 100);
    EXPECT_EQ(squareLead.name, "Synth Lead (Square)");
    EXPECT_EQ(squareLead.waveMask, static_cast<std::uint8_t>(SID_WAVE_PULSE | SID_WAVE_TRI));

    const SIDPatchParams sawLead = MIDISoundBank::GetInstrumentParams(81, 0, 72, 100);
    EXPECT_EQ(sawLead.name, "Synth Lead (Sawtooth)");
    EXPECT_EQ(sawLead.waveMask, static_cast<std::uint8_t>(SID_WAVE_SAW | SID_WAVE_TRI));
    EXPECT_NE(sawLead.features & static_cast<std::uint16_t>(SIDFeatureFlags::RingMod), 0U);

    const SIDPatchParams apuPulse = MIDISoundBank::GetInstrumentParams(128, 0, 60, 100);
    EXPECT_EQ(apuPulse.name, "APU Pulse");
    EXPECT_EQ(apuPulse.waveMask, SID_WAVE_PULSE);
}

TEST(MIDISoundBankTest, PercussionAndChipCompatibility) {
    MIDISoundBank::EnsureLoaded();
    EXPECT_FALSE(MIDISoundBank::GetPercussionKits().empty());

    const SIDPatchParams kick = MIDISoundBank::GetInstrumentParams(0, 9, 36, 120, Hardware::SIDModel::MOS8580);
    EXPECT_EQ(kick.name, "Kick");
    EXPECT_EQ(kick.waveMask, SID_WAVE_TRI);
    EXPECT_NE(kick.features & static_cast<std::uint16_t>(SIDFeatureFlags::KickSlide), 0U);
    EXPECT_EQ(kick.features & static_cast<std::uint16_t>(SIDFeatureFlags::PCMSample), 0U);

    const SIDPatchParams kick6581 = MIDISoundBank::GetInstrumentParams(0, 9, 36, 120, Hardware::SIDModel::MOS6581);
    EXPECT_NE(kick6581.features & static_cast<std::uint16_t>(SIDFeatureFlags::PCMSample), 0U);

    const SIDPatchParams snare8580 = MIDISoundBank::GetInstrumentParams(0, 9, 38, 120, Hardware::SIDModel::MOS8580);
    EXPECT_EQ(snare8580.name, "Snare");
    EXPECT_EQ(snare8580.waveMask, static_cast<std::uint8_t>(SID_WAVE_NOISE | SID_WAVE_TRI));

    const SIDPatchParams snare6581 = MIDISoundBank::GetInstrumentParams(0, 9, 38, 120, Hardware::SIDModel::MOS6581);
    EXPECT_EQ(snare6581.waveMask, SID_WAVE_NOISE);
}

TEST(MIDISoundBankTest, WaveformFormattingAndVelocityScaling) {
    EXPECT_EQ(MIDISoundBank::FormatWaveformMask(SID_WAVE_SAW | SID_WAVE_PULSE), "SAW+PULSE");
    EXPECT_EQ(MIDISoundBank::FormatWaveformMask(SID_WAVE_PULSE | SID_WAVE_TRI), "PULSE+TRI");
    EXPECT_EQ(MIDISoundBank::FormatWaveformMask(SID_WAVE_SAW | SID_WAVE_TRI), "SAW+TRI");

    const SIDPatchParams overridden =
        MIDISoundBank::GetInstrumentParams(0, 0, 60, 100, Hardware::SIDModel::MOS8580, SID_WAVE_SAW | SID_WAVE_TRI);
    EXPECT_EQ(overridden.waveMask, static_cast<std::uint8_t>(SID_WAVE_SAW | SID_WAVE_TRI));

    const SIDPatchParams lowVel = MIDISoundBank::GetInstrumentParams(0, 0, 60, 1);
    const int lowSusNibble = (lowVel.effectiveSustainRelease >> 4) & 0x0F;
    EXPECT_EQ(lowSusNibble, 4);

    const SIDPatchParams maxVel = MIDISoundBank::GetInstrumentParams(0, 0, 60, 127);
    const int maxSusNibble = (maxVel.effectiveSustainRelease >> 4) & 0x0F;
    EXPECT_EQ(maxSusNibble, 15);
}

TEST(MIDIFileTest, MainTrackSelectionAndSerializeRoundtrip) {
    MIDIProject project;
    project.InitDefaultProject();
    ASSERT_EQ(project.tracks.size(), 4U);
    EXPECT_TRUE(project.tracks[0].isMainTrack);
    EXPECT_EQ(project.tracks[0].role, MIDITrackRole::Melody);
    EXPECT_EQ(project.GetMainTrackIndex(), 0);

    project.SetMainTrack(2);
    EXPECT_FALSE(project.tracks[0].isMainTrack);
    EXPECT_TRUE(project.tracks[2].isMainTrack);
    EXPECT_EQ(project.tracks[2].role, MIDITrackRole::Melody);
    EXPECT_EQ(project.GetMainTrackIndex(), 2);

    project.bpm = 140.0;
    project.timeSigNumerator = 3;
    project.timeSigDenominator = 4;
    project.tracks[2].customWaveOverride = static_cast<std::uint8_t>(SID_WAVE_SAW | SID_WAVE_PULSE);
    project.tracks[2].notes.push_back(MIDINote{
        .id = project.nextNoteId++,
        .pitch = 72,
        .velocity = 115,
        .startBeat = 0.0,
        .durationBeats = 1.5,
    });
    project.tracks[1].notes.push_back(MIDINote{
        .id = project.nextNoteId++,
        .pitch = 36,
        .velocity = 100,
        .startBeat = 0.5,
        .durationBeats = 1.0,
    });

    const std::vector<std::uint8_t> bytes = MIDIFile::SerializeToBytes(project);
    ASSERT_FALSE(bytes.empty());

    MIDIProject loaded;
    std::string err;
    ASSERT_TRUE(MIDIFile::ParseFromBytes(bytes, loaded, err)) << err;

    EXPECT_NEAR(loaded.bpm, 140.0, 0.5);
    EXPECT_EQ(loaded.timeSigNumerator, 3);
    EXPECT_EQ(loaded.timeSigDenominator, 4);

    int loadedMainIdx = loaded.GetMainTrackIndex();
    ASSERT_GE(loadedMainIdx, 0);
    const MIDITrack& mainTrack = loaded.tracks[static_cast<std::size_t>(loadedMainIdx)];
    EXPECT_TRUE(mainTrack.isMainTrack);
    EXPECT_EQ(mainTrack.role, MIDITrackRole::Melody);
    EXPECT_EQ(mainTrack.customWaveOverride, static_cast<std::uint8_t>(SID_WAVE_SAW | SID_WAVE_PULSE));
    ASSERT_EQ(mainTrack.notes.size(), 1U);
    EXPECT_EQ(mainTrack.notes[0].pitch, 72);
    EXPECT_EQ(mainTrack.notes[0].velocity, 115);
    EXPECT_NEAR(mainTrack.notes[0].startBeat, 0.0, 0.01);
    EXPECT_NEAR(mainTrack.notes[0].durationBeats, 1.5, 0.01);

    bool foundBassTrack = false;
    for (const auto& t : loaded.tracks) {
        if (t.role == MIDITrackRole::Bass) {
            foundBassTrack = true;
            ASSERT_EQ(t.notes.size(), 1U);
            EXPECT_EQ(t.notes[0].pitch, 36);
        }
    }
    EXPECT_TRUE(foundBassTrack);
}

TEST(MIDIFileTest, PolyphonyCullingRespectsMainTrackPriority) {
    MIDIProject project;
    project.InitDefaultProject();

    project.tracks[0].notes.push_back(MIDINote{.id = 1, .pitch = 72, .velocity = 100, .startBeat = 0.0, .durationBeats = 1.0});
    project.tracks[1].notes.push_back(MIDINote{.id = 2, .pitch = 36, .velocity = 100, .startBeat = 0.0, .durationBeats = 1.0});
    project.tracks[2].notes.push_back(MIDINote{.id = 3, .pitch = 60, .velocity = 100, .startBeat = 0.0, .durationBeats = 1.0});
    project.tracks[3].notes.push_back(MIDINote{.id = 4, .pitch = 38, .velocity = 100, .startBeat = 0.0, .durationBeats = 0.5});

    MIDIFile::ComputeVoicePreviewAllocation(project);

    EXPECT_GE(project.tracks[0].notes[0].assignedSidVoice, 0);
    EXPECT_LE(project.tracks[0].notes[0].assignedSidVoice, 2);
    EXPECT_GE(project.tracks[1].notes[0].assignedSidVoice, 0);
    EXPECT_GE(project.tracks[3].notes[0].assignedSidVoice, 0);
    EXPECT_EQ(project.tracks[2].notes[0].assignedSidVoice, -1);
}

}  // namespace
