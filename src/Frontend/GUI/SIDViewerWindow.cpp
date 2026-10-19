#include "Frontend/GUI/SIDViewerWindow.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <format>
#include <numbers>
#include <random>
#include <span>
#include <string_view>
#include <vector>

#include "Frontend/GUI/Style/Style.h"
#include "Hardware/Audio/SIDWaveTables.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static void DrawMuteSoloButtons(Hardware::SID& sid, int voiceIndex) {
    ImGui::PushID(voiceIndex);
    bool isMuted = sid.IsVoiceMuted(voiceIndex);
    bool isSolo = sid.IsVoiceSolo(voiceIndex);

    if (isMuted) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85F, 0.20F, 0.20F, 0.9F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95F, 0.30F, 0.30F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75F, 0.15F, 0.15F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BorderSubtle);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::BorderSubtle);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0F, 0.0F));
    if (ImGui::SmallButton("M")) {
        sid.SetVoiceMute(voiceIndex, !isMuted);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    ImGui::SameLine(0.0F, 2.0F);

    if (isSolo) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90F, 0.70F, 0.10F, 0.9F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00F, 0.80F, 0.20F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.80F, 0.60F, 0.05F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1F, 0.1F, 0.1F, 1.0F));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BorderSubtle);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::BorderSubtle);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0F, 0.0F));
    if (ImGui::SmallButton("S")) {
        sid.SetVoiceSolo(voiceIndex, !isSolo);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    ImGui::PopID();
}

static void DrawVoiceWaveform(Hardware::SID& sid, int voiceIndex, double sid_time, bool should_update) {
    const auto& voice = sid.GetVoice(voiceIndex);

    ImGui::BeginGroup();
    ImGui::Text("V%d", voiceIndex + 1);
    ImGui::SameLine(0.0F, 4.0F);
    DrawMuteSoloButtons(sid, voiceIndex);

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

static void DrawColoredText(const ImVec4& color, std::string_view text) {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    ImGui::PopStyleColor();
}

static void DrawControlBadge(const char* label, bool active) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_Border, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_Text, ImColor(Theme::LcdPetroleumBg).Value);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::Surface);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::Surface);
        ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderSubtle);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0F, 1.0F));

    ImGui::Button(label);

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

static void DrawVoiceADSR(const Oscillator& voice) {
    const ImVec2 size(80.0F, 24.0F);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(p0, p1, ImColor(Theme::Surface));
    drawList->AddRect(p0, p1, ImColor(Theme::BorderSubtle), 2.0F);

    const uint8_t rateA = std::max(voice.env.attackRate, voice.env.targetAttackRate);
    const uint8_t rateD = std::max(voice.env.decayRate, voice.env.targetDecayRate);
    const uint8_t rateS = std::max(voice.env.sustainLevel, voice.env.targetSustainLevel);
    const uint8_t rateR = std::max(voice.env.releaseRate, voice.env.targetReleaseRate);

    const float y0 = p1.y - 3.0F;
    const float yPeak = p0.y + 3.0F;
    const float x0 = p0.x + 3.0F;
    const float x4 = p1.x - 3.0F;
    const float totalW = x4 - x0;

    const bool isConfigured = (rateA != 0 || rateD != 0 || rateS != 0 || rateR != 0 || voice.env.level != 0);
    const ImU32 curveColor = IM_COL32(110, 130, 150, 160);

    if (!isConfigured) {
        drawList->AddLine(ImVec2(x0, y0), ImVec2(x4, y0), curveColor, 1.2F);
        ImGui::Dummy(size);
        return;
    }

    const float weightA = 1.0F + (static_cast<float>(rateA) * 1.8F);
    const float weightD = 1.0F + (static_cast<float>(rateD) * 1.8F);
    const float weightS = 6.0F;
    const float weightR = 1.0F + (static_cast<float>(rateR) * 1.8F);
    const float totalWeight = weightA + weightD + weightS + weightR;

    const float x1 = x0 + (totalW * (weightA / totalWeight));
    const float x2 = x1 + (totalW * (weightD / totalWeight));
    const float x3 = x2 + (totalW * (weightS / totalWeight));

    const float susRatio = static_cast<float>(rateS) / 15.0F;
    const float ySus = y0 - ((y0 - yPeak) * susRatio);

    drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, yPeak), curveColor, 1.2F);
    drawList->AddLine(ImVec2(x1, yPeak), ImVec2(x2, ySus), curveColor, 1.2F);
    drawList->AddLine(ImVec2(x2, ySus), ImVec2(x3, ySus), curveColor, 1.2F);
    drawList->AddLine(ImVec2(x3, ySus), ImVec2(x4, y0), curveColor, 1.2F);

    const float curLevel = static_cast<float>(voice.env.level) / 255.0F;
    float curX = x0;
    float curY = y0 - ((y0 - yPeak) * curLevel);

    switch (voice.env.state) {
        case ADSREnvelope::ATTACK:
            curX = x0 + ((x1 - x0) * curLevel);
            curY = y0 - ((y0 - yPeak) * curLevel);
            break;
        case ADSREnvelope::DECAY: {
            float range = std::max(0.01F, 1.0F - susRatio);
            float t = std::clamp((1.0F - curLevel) / range, 0.0F, 1.0F);
            curX = x1 + ((x2 - x1) * t);
            curY = yPeak + ((ySus - yPeak) * t);
            break;
        }
        case ADSREnvelope::SUSTAIN:
            curX = (x2 + x3) * 0.5F;
            curY = ySus;
            break;
        case ADSREnvelope::RELEASE: {
            float range = (susRatio > 0.01F) ? susRatio : 1.0F;
            float t = std::clamp(1.0F - (curLevel / range), 0.0F, 1.0F);
            curX = x3 + ((x4 - x3) * t);
            curY = ySus + ((y0 - ySus) * t);
            break;
        }
        case ADSREnvelope::IDLE:
        default:
            curX = x0;
            curY = y0;
            break;
    }

    if (voice.env.level > 0 || voice.env.state != ADSREnvelope::IDLE) {
        const ImU32 activeColor = ImColor(Theme::AccentActive);

        if (voice.env.state == ADSREnvelope::ATTACK) {
            drawList->AddLine(ImVec2(x0, y0), ImVec2(curX, curY), activeColor, 2.0F);
        } else if (voice.env.state == ADSREnvelope::DECAY) {
            drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, yPeak), activeColor, 2.0F);
            drawList->AddLine(ImVec2(x1, yPeak), ImVec2(curX, curY), activeColor, 2.0F);
        } else if (voice.env.state == ADSREnvelope::SUSTAIN) {
            drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, yPeak), activeColor, 2.0F);
            drawList->AddLine(ImVec2(x1, yPeak), ImVec2(x2, ySus), activeColor, 2.0F);
            drawList->AddLine(ImVec2(x2, ySus), ImVec2(curX, ySus), activeColor, 2.0F);
        } else if (voice.env.state == ADSREnvelope::RELEASE) {
            drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, yPeak), activeColor, 2.0F);
            drawList->AddLine(ImVec2(x1, yPeak), ImVec2(x2, ySus), activeColor, 2.0F);
            drawList->AddLine(ImVec2(x2, ySus), ImVec2(x3, ySus), activeColor, 2.0F);
            drawList->AddLine(ImVec2(x3, ySus), ImVec2(curX, curY), activeColor, 2.0F);
        }

        drawList->AddLine(ImVec2(curX, y0), ImVec2(curX, curY), ImColor(Theme::AccentPrimary), 1.5F);
        drawList->AddCircleFilled(ImVec2(curX, curY), 4.0F, IM_COL32(79, 195, 247, 100));
        drawList->AddCircleFilled(ImVec2(curX, curY), 2.5F, activeColor);
    }

    ImGui::Dummy(size);
}

static void DrawVoiceParameters(const Core::SID& sid, int voiceIndex) {
    const auto& voice = sid.GetVoice(voiceIndex);

    ImGui::BeginGroup();
    DrawColoredText(Theme::TextSecondary, "Freq:");
    ImGui::SameLine(0.0F, 4.0F);
    DrawColoredText(Theme::AccentPrimary, std::format("{:04X}", voice.frequency));
    ImGui::SameLine();
    DrawColoredText(Theme::TextSecondary, "PW:");
    ImGui::SameLine(0.0F, 4.0F);
    DrawColoredText(Theme::AccentPrimary, std::format("{:03X}", voice.pulseWidth));
    ImGui::SameLine();
    DrawColoredText(Theme::TextMain, std::format("({})", GetWaveformText(voice.control)));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0F, 1.0F));
    ImGui::AlignTextToFramePadding();
    ImGui::PopStyleVar();

    const uint8_t dispA = std::max(voice.env.attackRate, voice.env.targetAttackRate);
    const uint8_t dispD = std::max(voice.env.decayRate, voice.env.targetDecayRate);
    const uint8_t dispS = std::max(voice.env.sustainLevel, voice.env.targetSustainLevel);
    const uint8_t dispR = std::max(voice.env.releaseRate, voice.env.targetReleaseRate);

    DrawColoredText(Theme::TextSecondary, "A:");
    ImGui::SameLine(0.0F, 2.0F);
    DrawColoredText(Theme::TextMain, std::format("{:X}", dispA));
    ImGui::SameLine();
    DrawColoredText(Theme::TextSecondary, "D:");
    ImGui::SameLine(0.0F, 2.0F);
    DrawColoredText(Theme::TextMain, std::format("{:X}", dispD));
    ImGui::SameLine();
    DrawColoredText(Theme::TextSecondary, "S:");
    ImGui::SameLine(0.0F, 2.0F);
    DrawColoredText(Theme::TextMain, std::format("{:X}", dispS));
    ImGui::SameLine();
    DrawColoredText(Theme::TextSecondary, "R:");
    ImGui::SameLine(0.0F, 2.0F);
    DrawColoredText(Theme::TextMain, std::format("{:X}", dispR));

    ImGui::SameLine(0.0F, 6.0F);
    DrawVoiceADSR(voice);

    ImGui::SameLine(0.0F, 6.0F);
    DrawControlBadge("GATE", (voice.control & 1) != 0);
    ImGui::SameLine();
    DrawControlBadge("SYNC", (voice.control & 2) != 0);
    ImGui::SameLine();
    DrawControlBadge("RING", (voice.control & 4) != 0);
    ImGui::SameLine();
    DrawControlBadge("TEST", (voice.control & 8) != 0);
    ImGui::EndGroup();
}

static void DrawPianoRoll(const Core::SID& sid) {
    ImGui::TextUnformatted("Piano Roll:");
    ImGui::SameLine();

    static constexpr const char* NOTE_NAMES[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    static constexpr std::array<ImU32, 3> VOICE_COLORS = {
        IM_COL32(79, 195, 247, 240),
        IM_COL32(255, 183, 77, 240),
        IM_COL32(240, 98, 146, 240)
    };

    std::array<int, 3> activeNotes = {-1, -1, -1};
    for (int v = 0; v < 3; ++v) {
        const auto& voice = sid.GetVoice(v);
        if (voice.frequency > 0 && voice.env.level > 0 && (voice.control & 0x08) == 0) {
            float freqHz = static_cast<float>(voice.frequency) * (1000000.0F / 16777216.0F);
            if (freqHz >= 16.0F) {
                activeNotes.at(v) = static_cast<int>(std::round(69.0F + (12.0F * std::log2(freqHz / 440.0F))));
            }
        }
        if (v > 0) {
            ImGui::SameLine(0.0F, 6.0F);
        }
        if (activeNotes.at(v) >= 0) {
            int noteVal = activeNotes.at(v);
            int nInOct = ((noteVal % 12) + 12) % 12;
            int oct = (noteVal / 12) - 1;
            float freqHz = static_cast<float>(sid.GetVoice(v).frequency) * (1000000.0F / 16777216.0F);
            ImGui::TextColored(ImColor(VOICE_COLORS.at(v)).Value, "V%d: %s%d (%.0fHz)", v + 1, NOTE_NAMES[nInOct], oct, freqHz);
        } else {
            ImGui::TextColored(Theme::TextSecondary, "V%d: ---", v + 1);
        }
    }

    constexpr int START_NOTE = 36;
    constexpr int NUM_WHITE_KEYS = 28;
    constexpr float WHITE_KEY_W = 8.0F;
    constexpr float WHITE_KEY_H = 15.0F;
    constexpr float BLACK_KEY_W = 5.0F;
    constexpr float BLACK_KEY_H = 9.0F;

    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static constexpr std::array<int, 12> NOTE_TO_WHITE = {
        0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6
    };

    for (int i = 0; i < NUM_WHITE_KEYS; ++i) {
        ImVec2 kp0(p0.x + (static_cast<float>(i) * WHITE_KEY_W), p0.y);
        ImVec2 kp1(kp0.x + WHITE_KEY_W, kp0.y + WHITE_KEY_H);
        drawList->AddRectFilled(kp0, kp1, IM_COL32(230, 230, 230, 255));
        drawList->AddRect(kp0, kp1, IM_COL32(60, 60, 60, 255));
    }

    for (int oct = 0; oct < 4; ++oct) {
        int baseW = oct * 7;
        const std::array<int, 5> blackOffsets = {0, 1, 3, 4, 5};
        for (int b : blackOffsets) {
            float kx = p0.x + (static_cast<float>(baseW + b) * WHITE_KEY_W) + (WHITE_KEY_W * 0.65F);
            ImVec2 kp0(kx, p0.y);
            ImVec2 kp1(kx + BLACK_KEY_W, p0.y + BLACK_KEY_H);
            drawList->AddRectFilled(kp0, kp1, IM_COL32(35, 35, 35, 255));
        }
    }

    for (int v = 0; v < 3; ++v) {
        int note = activeNotes.at(v);
        if (note < START_NOTE || note >= (START_NOTE + 48)) {
            continue;
        }
        int relNote = note - START_NOTE;
        int oct = relNote / 12;
        int nInOct = relNote % 12;
        int whiteIdx = NOTE_TO_WHITE.at(nInOct);

        if (whiteIdx >= 0) {
            int keyIdx = (oct * 7) + whiteIdx;
            ImVec2 kp0(p0.x + (static_cast<float>(keyIdx) * WHITE_KEY_W), p0.y + BLACK_KEY_H);
            ImVec2 kp1(kp0.x + WHITE_KEY_W, p0.y + WHITE_KEY_H);
            drawList->AddRectFilled(kp0, kp1, VOICE_COLORS.at(v));
        } else {
            int prevWhite = NOTE_TO_WHITE.at(nInOct - 1);
            int keyIdx = (oct * 7) + prevWhite;
            float kx = p0.x + (static_cast<float>(keyIdx) * WHITE_KEY_W) + (WHITE_KEY_W * 0.65F);
            ImVec2 kp0(kx, p0.y);
            ImVec2 kp1(kx + BLACK_KEY_W, p0.y + BLACK_KEY_H);
            drawList->AddRectFilled(kp0, kp1, VOICE_COLORS.at(v));
        }
    }

    ImGui::Dummy(ImVec2(static_cast<float>(NUM_WHITE_KEYS) * WHITE_KEY_W, WHITE_KEY_H));
}

static void ComputeFFT(std::vector<std::complex<float>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1U;
        for (; (j & bit) != 0; bit >>= 1U) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }
    for (size_t len = 2; len <= n; len <<= 1U) {
        float ang = -2.0F * std::numbers::pi_v<float> / static_cast<float>(len);
        std::complex<float> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0F, 0.0F);
            for (size_t j = 0; j < (len / 2U); ++j) {
                std::complex<float> u = a[i + j];
                std::complex<float> v = a[i + j + (len / 2U)] * w;
                a[i + j] = u + v;
                a[i + j + (len / 2U)] = u - v;
                w *= wlen;
            }
        }
    }
}

static void ProcessFFT(const Hardware::SID& sid, std::array<float, 32>& spectrumOut, bool should_update) {
    static std::array<float, 32> smoothedSpectrum{};
    if (!should_update) {
        for (size_t i = 0; i < 32; ++i) {
            smoothedSpectrum.at(i) *= 0.88F;
            if (smoothedSpectrum.at(i) < 0.005F) {
                smoothedSpectrum.at(i) = 0.0F;
            }
            spectrumOut.at(i) = smoothedSpectrum.at(i);
        }
        return;
    }

    constexpr size_t FFT_SIZE = 128;
    std::array<float, FFT_SIZE> samples{};
    sid.GetAudioSnapshot(samples);

    std::vector<std::complex<float>> fftData(FFT_SIZE);
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        float window = 0.5F * (1.0F - std::cos(2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / (FFT_SIZE - 1.0F)));
        fftData[i] = std::complex<float>(samples.at(i) * window, 0.0F);
    }
    ComputeFFT(fftData);

    for (size_t i = 0; i < 32; ++i) {
        float mag = (std::abs(fftData.at(i + 1)) / (static_cast<float>(FFT_SIZE) * 0.5F)) * 1.5F;
        smoothedSpectrum.at(i) = std::clamp(std::max(mag, smoothedSpectrum.at(i) * 0.82F), 0.0F, 1.0F);
        spectrumOut.at(i) = smoothedSpectrum.at(i);
    }
}

static void DrawCompactSpectrum(const std::array<float, 32>& spectrum) {
    const float availW = ImGui::GetContentRegionAvail().x;
    constexpr float barH = 14.0F;
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(p0, ImVec2(p0.x + availW, p0.y + barH), ImColor(Theme::Surface));
    const float barW = (availW - 4.0F) / 32.0F;
    const float maxH = barH - 2.0F;
    for (size_t i = 0; i < 32; ++i) {
        float h = std::clamp(spectrum.at(i) * maxH, 0.0F, maxH);
        float bx0 = p0.x + 2.0F + (static_cast<float>(i) * barW);
        float by1 = p0.y + barH - 1.0F;
        drawList->AddRectFilled(ImVec2(bx0, by1 - h), ImVec2(bx0 + barW - 1.0F, by1), ImColor(Theme::AccentPrimary));
    }
    ImGui::Dummy(ImVec2(availW, barH));
}

static void DrawFullSpectrogramTab(Hardware::SID& sid, const std::array<float, 32>& spectrum, bool should_update) {
    ImGui::PushID("FullSpectrogramTab");
    ImGui::TextUnformatted("Mute/Solo:");
    for (int v = 0; v < 3; ++v) {
        ImGui::SameLine(0.0F, 12.0F);
        ImGui::Text("V%d", v + 1);
        ImGui::SameLine(0.0F, 4.0F);
        DrawMuteSoloButtons(sid, v);
    }

    ImGui::Separator();

    ImGui::TextUnformatted("Frequency Spectrum:");
    const float availW = ImGui::GetContentRegionAvail().x;
    const float barHeight = 42.0F;
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(p0, ImVec2(p0.x + availW, p0.y + barHeight), ImColor(Theme::Surface));
    drawList->AddRect(p0, ImVec2(p0.x + availW, p0.y + barHeight), ImColor(Theme::BorderSubtle));

    const float barWidth = (availW - 4.0F) / 32.0F;
    const float maxBarH = barHeight - 6.0F;
    for (size_t i = 0; i < 32; ++i) {
        float h = std::clamp(spectrum.at(i) * maxBarH, 0.0F, maxBarH);
        float bx0 = p0.x + 2.0F + (static_cast<float>(i) * barWidth);
        float by1 = p0.y + barHeight - 3.0F;
        float by0 = by1 - h;
        ImU32 col = ImColor(Theme::AccentPrimary);
        if (spectrum.at(i) > 0.7F) {
            col = IM_COL32(255, 100, 100, 255);
        } else if (spectrum.at(i) > 0.4F) {
            col = IM_COL32(255, 200, 50, 255);
        }
        drawList->AddRectFilled(ImVec2(bx0, by0), ImVec2(bx0 + barWidth - 1.0F, by1), col);
    }
    ImGui::Dummy(ImVec2(availW, barHeight + 4.0F));

    ImGui::TextUnformatted("Waterfall Spectrogram (Time vs Freq):");
    constexpr int WATERFALL_COLS = 32;
    constexpr int WATERFALL_ROWS = 28;
    static std::array<std::array<float, WATERFALL_COLS>, WATERFALL_ROWS> waterfallGrid{};
    if (should_update) {
        for (int r = WATERFALL_ROWS - 1; r > 0; --r) {
            waterfallGrid.at(r) = waterfallGrid.at(r - 1);
        }
        waterfallGrid.at(0) = spectrum;
    }

    const ImVec2 wp0 = ImGui::GetCursorScreenPos();
    const float availH = ImGui::GetContentRegionAvail().y;
    const float totalWaterfallH = std::max(60.0F, availH - 4.0F);
    const float cellW = availW / static_cast<float>(WATERFALL_COLS);
    const float cellH = totalWaterfallH / static_cast<float>(WATERFALL_ROWS);

    drawList->AddRectFilled(wp0, ImVec2(wp0.x + availW, wp0.y + totalWaterfallH), ImColor(Theme::Surface));
    drawList->AddRect(wp0, ImVec2(wp0.x + availW, wp0.y + totalWaterfallH), ImColor(Theme::BorderSubtle));

    for (int r = 0; r < WATERFALL_ROWS; ++r) {
        for (size_t c = 0; c < WATERFALL_COLS; ++c) {
            float val = waterfallGrid.at(r).at(c);
            float cx0 = wp0.x + (static_cast<float>(c) * cellW);
            float cy0 = wp0.y + (static_cast<float>(r) * cellH);
            ImVec2 cp0(cx0 + 0.5F, cy0 + 0.5F);
            ImVec2 cp1(cx0 + cellW - 0.5F, cy0 + cellH - 0.5F);

            ImU32 cellColor = IM_COL32(11, 29, 40, 220);
            if (val > 0.75F) {
                cellColor = IM_COL32(255, 235, 59, 255);
            } else if (val > 0.50F) {
                cellColor = IM_COL32(0, 230, 200, 240);
            } else if (val > 0.25F) {
                cellColor = IM_COL32(0, 150, 180, 220);
            } else if (val > 0.05F) {
                cellColor = IM_COL32(20, 70, 95, 200);
            }
            drawList->AddRectFilled(cp0, cp1, cellColor);
        }
    }
    ImGui::Dummy(ImVec2(availW, totalWaterfallH));
    ImGui::PopID();
}

static void DrawSIDViewerContent(AppState& state, double sid_time, bool should_update) {
    std::array<float, 32> spectrum{};
    ProcessFFT(state.emulator.GetSID(), spectrum, should_update);

    if (ImGui::BeginTabBar("SIDViewerTabs")) {
        if (ImGui::BeginTabItem("Oscillators")) {
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

            ImGui::Separator();
            DrawPianoRoll(state.emulator.GetSID());
            ImGui::Spacing();
            DrawCompactSpectrum(spectrum);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("FFT Spectrogram")) {
            DrawFullSpectrogramTab(state.emulator.GetSID(), spectrum, should_update);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
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
