#include "Frontend/GUI/SIDViewerWindow.h"

#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>

#include "Hardware/Audio/SIDWaveTables.h"

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
    auto envLevel = static_cast<float>(voice.env.level) / 255.0F;

    for (size_t nIdx = 0; nIdx < 50; nIdx++) {
        float timeRatio = (float)nIdx / 49.0F;
        float phase = (timeRatio * freqScale) - (float)(time * speed * freqScale);
        phase -= std::floor(phase);

        float val = 0.0F;
        if (voice.frequency > 0 && envLevel > 0.0F && (voice.control & 0x08) == 0) {
            uint8_t waveControl = (voice.control >> 4) & 0x0F;
            bool noiseOn = (waveControl & 0x08) != 0;
            bool pulseOn = (waveControl & 0x04) != 0;
            bool sawOn = (waveControl & 0x02) != 0;
            bool triOn = (waveControl & 0x01) != 0;

            if (waveControl == 0) {
                val = 0.0F;
            } else if (noiseOn) {
                if (should_update) {
                    noise_buffer.at(static_cast<size_t>(voiceIndex)).at(nIdx) = dist(randomGenerator);
                }
                if (sid.GetModel() == SIDModel::MOS6581 && waveControl != 0x08) {
                    val = 0.0F;
                } else {
                    val = noise_buffer.at(static_cast<size_t>(voiceIndex)).at(nIdx);
                }
            } else {
                auto acc = static_cast<uint32_t>(phase * 16777216.0F) & 0xFFFFFF;
                auto ix = static_cast<uint16_t>(acc >> 12);
                uint16_t pulseOutput = (ix >= (voice.pulseWidth & 0xFFF)) ? 0xFFF : 0;
                uint32_t msb = acc & 0x800000;
                uint16_t tri = ((acc ^ (msb != 0 ? 0xFFFFFF : 0)) >> 11) & 0xFFF;
                uint16_t saw = ix;
                uint16_t out = 0;

                if (sid.GetModel() == SIDModel::MOS6581) {
                    if (waveControl == 0x01) {
                        out = tri;
                    } else if (waveControl == 0x02) {
                        out = saw;
                    } else if (waveControl == 0x04) {
                        out = pulseOutput;
                    } else if (waveControl == 0x03) {
                        out = wave6581__ST[ix];
                    } else if (waveControl == 0x05) {
                        out = wave6581_P_T[ix] & pulseOutput;
                    } else if (waveControl == 0x06) {
                        out = wave6581_PS_[ix] & pulseOutput;
                    } else if (waveControl == 0x07) {
                        out = wave6581_PST[ix] & pulseOutput;
                    }
                } else {
                    out = 0xFFF;
                    if (triOn) {
                        out &= tri;
                    }
                    if (sawOn) {
                        out &= saw;
                    }
                    if (pulseOn) {
                        out &= pulseOutput;
                    }
                }

                val = (static_cast<float>(out) / 2047.5F) - 1.0F;
            }
            val *= envLevel;
        }
        points.at(nIdx) = val;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::BeginChild(ImGui::GetID(voiceIndex), ImVec2(100, 30), 0, ImGuiWindowFlags_NoInputs);
    ImGui::PlotLines("##Wave", points.data(), 50, 0, nullptr, -1.1F, 1.1F, ImVec2(100, 30));
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();
}

[[nodiscard]] static std::string GetWaveformText(uint8_t control) {
    std::string wave;
    if ((control & 0x10) != 0) {
        wave += "TRI";
    }
    if ((control & 0x20) != 0) {
        if (!wave.empty()) {
            wave += " + ";
        }
        wave += "SAW";
    }
    if ((control & 0x40) != 0) {
        if (!wave.empty()) {
            wave += " + ";
        }
        wave += "PULSE";
    }
    if ((control & 0x80) != 0) {
        if (!wave.empty()) {
            wave += " + ";
        }
        wave += "NOISE";
    }
    if (wave.empty()) {
        wave = "OFF";
    }
    return wave;
}

static void DrawVoiceParameters(const Core::SID& sid, int voiceIndex) {
    const auto& voice = sid.GetVoice(voiceIndex);

    ImGui::BeginGroup();
    std::ostringstream freqOss;
    freqOss << "Freq: " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << voice.frequency
            << "  PW: " << std::setw(3) << voice.pulseWidth << " (" << GetWaveformText(voice.control) << ")";
    ImGui::TextUnformatted(freqOss.str().c_str());

    std::ostringstream envOss;
    envOss << "A:" << std::hex << std::uppercase << static_cast<int>(voice.env.attackRate)
           << " D:" << static_cast<int>(voice.env.decayRate) << " S:" << static_cast<int>(voice.env.sustainLevel)
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

static void DrawSIDViewerContent(const AppState& state, double sid_time, bool should_update) {
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
}

void DrawSIDViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                         ImGuiWindowFlags window_flags) {
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + (work_size.x * 0.47F), work_pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(work_size.x * 0.33F, top_section_height), ImGuiCond_Always);
    ImGui::Begin("SID Viewer", nullptr, window_flags | ImGuiWindowFlags_NoMove);

    state.render.sidWindowPos[0] = ImGui::GetWindowPos().x;
    state.render.sidWindowPos[1] = ImGui::GetWindowPos().y;
    state.render.sidWindowSize[0] = ImGui::GetWindowSize().x;
    state.render.sidWindowSize[1] = ImGui::GetWindowSize().y;

    static double sid_time = 0.0;
    if (state.emulator.IsRunning() && !state.emulator.IsPaused()) {
        sid_time += ImGui::GetIO().DeltaTime;
    }

    bool should_update = state.emulator.IsRunning() && !state.emulator.IsPaused();
    DrawSIDViewerContent(state, sid_time, should_update);

    ImGui::End();
}

}  // namespace GUI
