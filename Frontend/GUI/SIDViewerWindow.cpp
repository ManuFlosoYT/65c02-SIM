#include "Frontend/GUI/SIDViewerWindow.h"

#include <cmath>
#include <cstdlib>

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawSIDViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                         ImGuiWindowFlags window_flags) {
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + (work_size.x * 0.47F), work_pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(work_size.x * 0.33F, top_section_height), ImGuiCond_Always);
    ImGui::Begin("SID Viewer", nullptr, window_flags);

    static double sid_time = 0.0;
    if (state.emulator.IsRunning() && !state.emulator.IsPaused()) {
        sid_time += ImGui::GetIO().DeltaTime;
    }

    for (int i = 0; i < 3; ++i) {
        const auto& voice = state.emulator.GetSID().GetVoice(i);
        ImGui::PushID(i);
        if (i > 0) {
            ImGui::Separator();
        }

        // Left Column: Name + Graph
        ImGui::BeginGroup();
        ImGui::Text("Voice %d", i + 1);

        // Waveform visualization
        static std::array<std::array<float, 50>, 3> noise_buffer{};
        static bool initialized = false;

        if (!initialized) {
            for (int voiceIdx = 0; voiceIdx < 3; voiceIdx++) {
                for (int nIdx = 0; nIdx < 50; nIdx++) {
                    noise_buffer[voiceIdx][nIdx] = ((float)(rand() % 100) / 50.0F) - 1.0F;
                }
            }
            initialized = true;
        }

        bool should_update = state.emulator.IsRunning() && !state.emulator.IsPaused();

        std::array<float, 50> points{};
        double time = sid_time;
        float speed = 2.0F;
        float freqScale = 1.0F + ((float)voice.frequency / 4000.0F);

        for (int nIdx = 0; nIdx < 50; nIdx++) {
            float timeRatio = (float)nIdx / 49.0F;
            float phase = (timeRatio * freqScale) - (float)(time * speed * freqScale);
            phase -= std::floor(phase);

            float val = 0.0F;
            if ((voice.control & 0x10) != 0) {  // Triangle
                val = (phase < 0.5F) ? (-1.0F + (4.0F * phase)) : (3.0F - (4.0F * phase));
            } else if ((voice.control & 0x20) != 0) {  // Sawtooth
                val = (2.0F * phase) - 1.0F;
            } else if ((voice.control & 0x40) != 0) {  // Pulse
                float pw = (float)(voice.pulseWidth & 0xFFF) / 4095.0F;
                if (pw == 0.0F) {
                    pw = 0.5F;
                }
                val = (phase < pw) ? 1.0F : -1.0F;
            } else if ((voice.control & 0x80) != 0) {  // Noise
                if (should_update) {
                    noise_buffer[i][nIdx] = ((float)(rand() % 100) / 50.0F) - 1.0F;
                }
                val = noise_buffer[i][nIdx];
            }
            points[nIdx] = val;
        }
        ImGui::BeginChild(ImGui::GetID(i), ImVec2(100, 30), 0, ImGuiWindowFlags_NoInputs);
        ImGui::PlotLines("##Wave", points.data(), 50, 0, nullptr, -1.0F, 1.0F, ImVec2(100, 30));
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::SameLine();

        // Right Column: Parameters
        ImGui::BeginGroup();
        ImGui::Text("Freq: %04X  PW: %03X", voice.frequency, voice.pulseWidth);
        ImGui::Text("A:%X D:%X S:%X R:%X", voice.env.attackRate, voice.env.decayRate,
                    (int)(voice.env.sustainLevel * 15), voice.env.releaseRate);
        ImGui::SameLine();
        ImGui::Text("Ctrl: %02X (G:%d S:%d R:%d T:%d)", voice.control, (voice.control & 1), (voice.control & 2) >> 1,
                    (voice.control & 4) >> 2, (voice.control & 8) >> 3);
        ImGui::EndGroup();
        ImGui::PopID();
    }
    ImGui::End();
}

}  // namespace GUI
