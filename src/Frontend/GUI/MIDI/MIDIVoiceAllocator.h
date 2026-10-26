#pragma once

#include <cstdint>
#include <vector>
#include "Frontend/GUI/MIDI/MIDIFile.h"
#include "Hardware/Audio/SID.h"

namespace GUI::MIDI {

class MIDIVoiceAllocator {
   public:
    static MIDITrackRole ResolveTrackRole(MIDITrackRole role, int channel, int mainChannel = -1,
                                          int bassChannel = -1);

    static void DetectProjectChannels(const MIDIProject& project, int& outMainChannel,
                                      int& outBassChannel);

    static int ComputeNotePriority(int pitch, MIDITrackRole role, bool isMainTrack = false,
                                   int channel = 0);

    static int ComputeNotePriority(const MIDITrack& track, const MIDINote& note);

    static void AnalyzeAndAssignTrackRoles(MIDIProject& project);

    static void ComputeVoicePreviewAllocation(
        MIDIProject& project, Hardware::SIDModel chipModel = Hardware::SIDModel::MOS8580);
};

}  // namespace GUI::MIDI
