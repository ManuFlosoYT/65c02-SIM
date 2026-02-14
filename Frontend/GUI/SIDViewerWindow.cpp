#include "Frontend/GUI/SIDViewerWindow.h"

#include <cmath>
#include <cstdlib>

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawSIDViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size,
                         float top_section_height,
                         ImGuiWindowFlags window_flags) {
    if (!state.emulator.GetSID().IsSoundEnabled()) return;

    ImGui::SetNextWindowPos(
        ImVec2(work_pos.x + work_size.x * 0.375f, work_pos.y),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(work_size.x * 0.375f, top_section_height),
                             ImGuiCond_Always);
    ImGui::Begin("SID Viewer", nullptr, window_flags);

    for (int i = 0; i < 3; ++i) {
        const auto& voice = state.emulator.GetSID().GetVoice(i);
        ImGui::PushID(i);
        if (i > 0) ImGui::Separator();

        // Left Column: Name + Graph
        ImGui::BeginGroup();
        ImGui::Text("Voice %d", i + 1);

        // Waveform visualization
        float points[50];
        double time = ImGui::GetTime();
        float speed = 2.0f;
        float freqScale = 1.0f + (voice.frequency / 4000.0f);

        for (int n = 0; n < 50; n++) {
            float t = (float)n / 49.0f;
            float phase = (t * freqScale) - (float)(time * speed * freqScale);
            phase -= floor(phase);

            float val = 0.0f;
            if (voice.control & 0x10) {  // Triangle
                val = (phase < 0.5f) ? (-1.0f + 4.0f * phase)
                                     : (3.0f - 4.0f * phase);
            } else if (voice.control & 0x20) {  // Sawtooth
                val = 2.0f * phase - 1.0f;
            } else if (voice.control & 0x40) {  // Pulse
                float pw = (voice.pulseWidth & 0xFFF) / 4095.0f;
                if (pw == 0) pw = 0.5f;
                val = (phase < pw) ? 1.0f : -1.0f;
            } else if (voice.control & 0x80) {  // Noise
                val = ((float)(rand() % 100) / 50.0f) - 1.0f;
            }
            points[n] = val;
        }
        ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)i), ImVec2(100, 30),
                          false, ImGuiWindowFlags_NoInputs);
        ImGui::PlotLines("##Wave", points, 50, 0, nullptr, -1.0f, 1.0f,
                         ImVec2(100, 30));
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::SameLine();

        // Right Column: Parameters
        ImGui::BeginGroup();
        ImGui::Text("Freq: %04X  PW: %03X", voice.frequency, voice.pulseWidth);
        ImGui::Text("A:%X D:%X S:%X R:%X", voice.env.attackRate,
                    voice.env.decayRate, (int)(voice.env.sustainLevel * 15),
                    voice.env.releaseRate);
        ImGui::SameLine();
        ImGui::Text("Ctrl: %02X (G:%d S:%d R:%d T:%d)", voice.control,
                    (voice.control & 1), (voice.control & 2) >> 1,
                    (voice.control & 4) >> 2, (voice.control & 8) >> 3);
        ImGui::EndGroup();
        ImGui::PopID();
    }
    ImGui::End();
}

}  // namespace GUI
