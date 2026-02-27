#include "Frontend/GUI/SIDViewerWindow.h"

#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static void DrawVoiceWaveform(const Core::SID& sid, int voiceIndex, double sid_time, bool should_update) {
    const auto& voice = sid.GetVoice(voiceIndex);

    ImGui::BeginGroup();
    std::ostringstream nameOss;
    nameOss << "Voice " << (voiceIndex + 1);
    ImGui::TextUnformatted(nameOss.str().c_str());

    static std::array<std::array<float, 50>, 3> noise_buffer{};
    static bool initialized = false;
    static std::mt19937 randomGenerator{std::random_device{}()};
    static std::uniform_real_distribution<float> dist(-1.0F, 1.0F);

    if (!initialized) {
        for (size_t voiceIdx = 0; voiceIdx < 3; voiceIdx++) {
            for (size_t nIdx = 0; nIdx < 50; nIdx++) {
                noise_buffer.at(voiceIdx).at(nIdx) = dist(randomGenerator);
            }
        }
        initialized = true;
    }

    std::array<float, 50> points{};
    double time = sid_time;
    float speed = 2.0F;
    float freqScale = 1.0F + ((float)voice.frequency / 4000.0F);

    for (size_t nIdx = 0; nIdx < 50; nIdx++) {
        float timeRatio = (float)nIdx / 49.0F;
        float phase = (timeRatio * freqScale) - (float)(time * speed * freqScale);
        phase -= std::floor(phase);

        float val = 0.0F;
        if ((voice.control & 0x10) != 0) {  // Triangle
            val = (phase < 0.5F) ? (-1.0F + (4.0F * phase)) : (3.0F - (4.0F * phase));
        } else if ((voice.control & 0x20) != 0) {  // Sawtooth
            val = (2.0F * phase) - 1.0F;
        } else if ((voice.control & 0x40) != 0) {  // Pulse
            float pulseWidthRatio = (float)(voice.pulseWidth & 0xFFF) / 4095.0F;
            if (pulseWidthRatio == 0.0F) {
                pulseWidthRatio = 0.5F;
            }
            val = (phase < pulseWidthRatio) ? 1.0F : -1.0F;
        } else if ((voice.control & 0x80) != 0) {  // Noise
            if (should_update) {
                noise_buffer.at(static_cast<size_t>(voiceIndex)).at(nIdx) = dist(randomGenerator);
            }
            val = noise_buffer.at(static_cast<size_t>(voiceIndex)).at(nIdx);
        }
        points.at(nIdx) = val;
    }
    ImGui::BeginChild(ImGui::GetID(voiceIndex), ImVec2(100, 30), 0, ImGuiWindowFlags_NoInputs);
    ImGui::PlotLines("##Wave", points.data(), 50, 0, nullptr, -1.0F, 1.0F, ImVec2(100, 30));
    ImGui::EndChild();
    ImGui::EndGroup();
}

static void DrawVoiceParameters(const Core::SID& sid, int voiceIndex) {
    const auto& voice = sid.GetVoice(voiceIndex);

    ImGui::BeginGroup();
    std::ostringstream freqOss;
    freqOss << "Freq: " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << voice.frequency
            << "  PW: " << std::setw(3) << voice.pulseWidth;
    ImGui::TextUnformatted(freqOss.str().c_str());

    std::ostringstream envOss;
    envOss << "A:" << std::hex << std::uppercase << static_cast<int>(voice.env.attackRate)
           << " D:" << static_cast<int>(voice.env.decayRate) << " S:" << static_cast<int>(voice.env.sustainLevel * 15)
           << " R:" << static_cast<int>(voice.env.releaseRate);
    ImGui::TextUnformatted(envOss.str().c_str());

    ImGui::SameLine();
    std::ostringstream ctrlOss;
    ctrlOss << "Ctrl: " << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
            << static_cast<int>(voice.control) << std::dec << " (G:" << (voice.control & 1)
            << " S:" << ((voice.control & 2) >> 1) << " R:" << ((voice.control & 4) >> 2)
            << " T:" << ((voice.control & 8) >> 3) << ")";
    ImGui::TextUnformatted(ctrlOss.str().c_str());
    ImGui::EndGroup();
}

void DrawSIDViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                         ImGuiWindowFlags window_flags) {
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + (work_size.x * 0.47F), work_pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(work_size.x * 0.33F, top_section_height), ImGuiCond_Always);
    ImGui::Begin("SID Viewer", nullptr, window_flags);

    static double sid_time = 0.0;
    if (state.emulator.IsRunning() && !state.emulator.IsPaused()) {
        sid_time += ImGui::GetIO().DeltaTime;
    }

    bool should_update = state.emulator.IsRunning() && !state.emulator.IsPaused();
    for (int voiceIndex = 0; voiceIndex < 3; ++voiceIndex) {
        ImGui::PushID(voiceIndex);
        if (voiceIndex > 0) {
            ImGui::Separator();
        }

        DrawVoiceWaveform(state.emulator.GetSID(), voiceIndex, sid_time, should_update);
        ImGui::SameLine();
        DrawVoiceParameters(state.emulator.GetSID(), voiceIndex);

        ImGui::PopID();
    }
    ImGui::End();
}

}  // namespace GUI
