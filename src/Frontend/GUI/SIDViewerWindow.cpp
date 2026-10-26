#include "Frontend/GUI/SIDViewerWindow.h"
#include "Frontend/GUI/SIDAudioAnalyzer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <format>
#include <numbers>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Frontend/GUI/MIDI/MIDIWorkspaceWindow.h"
#include "Frontend/GUI/Style/Style.h"
#include "Hardware/Audio/SIDWaveTables.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static constexpr std::array<ImU32, 3> VOICE_COLORS = {
    IM_COL32(79, 195, 247, 240),
    IM_COL32(255, 183, 77, 240),
    IM_COL32(240, 98, 146, 240)
};

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
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(isMuted ? "Unmute this voice" : "Mute this voice");
        ImGui::EndTooltip();
    }

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
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(isSolo ? "Unsolo this voice" : "Solo this voice");
        ImGui::EndTooltip();
    }
    ImGui::PopID();
}

static void DrawVoiceWaveform(Hardware::SID& sid, int voiceIndex, double sid_time, bool should_update, bool phase_trigger) {
    const auto& voice = sid.GetVoice(voiceIndex);

    ImGui::BeginGroup();
    ImGui::TextColored(ImColor(VOICE_COLORS.at(voiceIndex)).Value, "V%d", voiceIndex + 1);
    ImGui::SameLine(0.0F, 4.0F);
    DrawMuteSoloButtons(sid, voiceIndex);

    constexpr size_t WAVE_POINTS = 80;
    static std::array<std::array<float, WAVE_POINTS>, 3> noise_buffer{};
    static bool initialized = false;
    static std::mt19937 randomGenerator{std::random_device{}()};
    static std::uniform_real_distribution<float> dist(-1.0F, 1.0F);

    if (!initialized) {
        for (size_t voiceIdx = 0; voiceIdx < 3; voiceIdx++) {
            for (size_t nIdx = 0; nIdx < WAVE_POINTS; nIdx++) {
                noise_buffer.at(voiceIdx).at(nIdx) = dist(randomGenerator);
            }
        }
        initialized = true;
    }

    std::array<float, WAVE_POINTS> points{};
    double time = sid_time;
    float speed = 2.0F;
    float freqScale = 1.0F + (static_cast<float>(voice.frequency) / 4000.0F);
    auto envLevel = static_cast<float>(voice.env.level) / 255.0F;

    for (size_t nIdx = 0; nIdx < WAVE_POINTS; nIdx++) {
        float timeRatio = static_cast<float>(nIdx) / static_cast<float>(WAVE_POINTS - 1);
        float phaseOffset = phase_trigger ? 0.0F : static_cast<float>(time * speed * freqScale);
        float phase = (timeRatio * freqScale) - phaseOffset;
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
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImColor(VOICE_COLORS.at(voiceIndex)).Value);
    ImGui::BeginChild(ImGui::GetID(voiceIndex), ImVec2(210.0F, 30.0F), 0, ImGuiWindowFlags_NoInputs);
    ImGui::PlotLines("##Wave", points.data(), static_cast<int>(WAVE_POINTS), 0, nullptr, -1.1F, 1.1F, ImVec2(210.0F, 30.0F));
    ImGui::EndChild();
    ImGui::PopStyleColor();
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
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0F, 1.5F));

    ImGui::Button(label);

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

static void DrawVoiceADSR(const Oscillator& voice, int voiceIndex) {
    const ImVec2 size(240.0F, 26.0F);
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
    const ImU32 vCol = VOICE_COLORS.at(voiceIndex);
    const ImU32 curveColor = (vCol & 0x00FFFFFF) | 0x70000000;

    if (!isConfigured) {
        drawList->AddLine(ImVec2(x0, y0), ImVec2(x4, y0), IM_COL32(110, 130, 150, 160), 1.2F);
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
        const ImU32 activeColor = VOICE_COLORS.at(voiceIndex);

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

        drawList->AddLine(ImVec2(curX, y0), ImVec2(curX, curY), activeColor, 1.5F);
        drawList->AddCircleFilled(ImVec2(curX, curY), 4.0F, (activeColor & 0x00FFFFFF) | 0x60000000);
        drawList->AddCircleFilled(ImVec2(curX, curY), 2.5F, activeColor);
    }

    ImGui::Dummy(size);
}

static void DrawVoiceADSRWarning(const Hardware::SID& sid, int voiceIndex, bool enabled) {
    const auto& voice = sid.GetVoice(voiceIndex);
    const ImVec2 size(24.0F, 26.0F);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static std::array<uint64_t, 3> lastBugClock = {0, 0, 0};
    static std::array<double, 3> bugAlertTime = {-10.0, -10.0, -10.0};

    const uint64_t currentBugClock = voice.GetADSRDelayBugClock();
    if (currentBugClock == 0) {
        lastBugClock.at(voiceIndex) = 0;
        bugAlertTime.at(voiceIndex) = -10.0;
    } else if (currentBugClock != lastBugClock.at(voiceIndex)) {
        lastBugClock.at(voiceIndex) = currentBugClock;
        bugAlertTime.at(voiceIndex) = ImGui::GetTime();
    }

    const double now = ImGui::GetTime();
    const bool isWarning = enabled && (now - bugAlertTime.at(voiceIndex) < 2.0);

    if (isWarning) {
        const bool blinkVisible = (static_cast<int>(now * 4.0) % 2 == 0);
        if (blinkVisible) {
            const ImU32 warnColor = IM_COL32(255, 180, 0, 255);
            const char* text = "(!)";
            const ImVec2 textSize = ImGui::CalcTextSize(text);
            const ImVec2 textPos = ImVec2(
                p0.x + ((size.x - textSize.x) * 0.5F),
                p0.y + ((size.y - textSize.y) * 0.5F)
            );
            drawList->AddText(textPos, warnColor, text);
        }

        ImGui::PushID(voiceIndex);
        ImGui::InvisibleButton("##ADSRWarn", size);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0F, 0.75F, 0.0F, 1.0F), "ADSR Delay Bug / Missing Hard Restart");
            ImGui::Separator();
            ImGui::TextUnformatted("Gate Off -> Gate On occurred in < 2 frames without ADSR = $0000.");
            ImGui::TextUnformatted("The envelope generator may desync or start from an unreleased level,");
            ImGui::TextUnformatted("causing choked notes or missing attack.");
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    } else {
        ImGui::Dummy(size);
    }
}

static void DrawVoiceParameters(const Core::SID& sid, int voiceIndex, bool showADSRWarning) {
    const auto& voice = sid.GetVoice(voiceIndex);
    const ImVec4 voiceColor = ImColor(VOICE_COLORS.at(voiceIndex)).Value;

    ImGui::BeginGroup();
    DrawColoredText(Theme::TextSecondary, "Freq:");
    ImGui::SameLine(0.0F, 4.0F);
    DrawColoredText(voiceColor, std::format("{:04X}", voice.frequency));
    ImGui::SameLine(0.0F, 34.0F);
    DrawColoredText(Theme::TextSecondary, "PW:");
    ImGui::SameLine(0.0F, 4.0F);
    const float dutyPct = (static_cast<float>(voice.pulseWidth & 0x0FFFU) / 4095.0F) * 100.0F;
    DrawColoredText(voiceColor, std::format("{:03X} ({:.0f}%)", voice.pulseWidth, dutyPct));
    ImGui::SameLine(0.0F, 34.0F);
    DrawColoredText(Theme::TextSecondary, "Wave:");
    ImGui::SameLine(0.0F, 4.0F);
    DrawColoredText(voiceColor, GetWaveformText(voice.control));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0F, 1.0F));
    ImGui::AlignTextToFramePadding();
    ImGui::PopStyleVar();

    const uint8_t dispA = std::max(voice.env.attackRate, voice.env.targetAttackRate);
    const uint8_t dispD = std::max(voice.env.decayRate, voice.env.targetDecayRate);
    const uint8_t dispS = std::max(voice.env.sustainLevel, voice.env.targetSustainLevel);
    const uint8_t dispR = std::max(voice.env.releaseRate, voice.env.targetReleaseRate);

    DrawColoredText(Theme::TextSecondary, "A:");
    ImGui::SameLine(0.0F, 2.0F);
    DrawColoredText(voiceColor, std::format("{:X}", dispA));
    ImGui::SameLine(0.0F, 12.0F);
    DrawColoredText(Theme::TextSecondary, "D:");
    ImGui::SameLine(0.0F, 2.0F);
    DrawColoredText(voiceColor, std::format("{:X}", dispD));
    ImGui::SameLine(0.0F, 12.0F);
    DrawColoredText(Theme::TextSecondary, "S:");
    ImGui::SameLine(0.0F, 2.0F);
    DrawColoredText(voiceColor, std::format("{:X}", dispS));
    ImGui::SameLine(0.0F, 12.0F);
    DrawColoredText(Theme::TextSecondary, "R:");
    ImGui::SameLine(0.0F, 2.0F);
    DrawColoredText(voiceColor, std::format("{:X}", dispR));

    ImGui::SameLine(0.0F, 26.0F);
    DrawVoiceADSR(voice, voiceIndex);

    ImGui::SameLine(0.0F, 4.0F);
    DrawVoiceADSRWarning(sid, voiceIndex, showADSRWarning);
    ImGui::SameLine(0.0F, 4.0F);

    DrawControlBadge("GATE", (voice.control & 1) != 0);
    ImGui::SameLine(0.0F, 10.0F);
    DrawControlBadge("SYNC", (voice.control & 2) != 0);
    ImGui::SameLine(0.0F, 10.0F);
    DrawControlBadge("RING", (voice.control & 4) != 0);
    ImGui::SameLine(0.0F, 10.0F);
    DrawControlBadge("TEST", (voice.control & 8) != 0);
    ImGui::EndGroup();
}

static void DrawPianoRoll(const Core::SID& sid) {
    const float fullAvailW = ImGui::GetContentRegionAvail().x;
    ImGui::TextUnformatted("Piano Roll:");

    static constexpr const char* NOTE_NAMES[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    std::array<int, 3> activeNotes = {-1, -1, -1};
    for (int v = 0; v < 3; ++v) {
        ImGui::SameLine(0.0F, 28.0F);
        const auto& voice = sid.GetVoice(v);
        if (voice.frequency > 0 && voice.env.level > 0 && (voice.control & 0x08) == 0) {
            float freqHz = static_cast<float>(voice.frequency) * (1000000.0F / 16777216.0F);
            if (freqHz >= 16.0F) {
                activeNotes.at(v) = static_cast<int>(std::round(69.0F + (12.0F * std::log2(freqHz / 440.0F))));
            }
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

    constexpr int START_NOTE = 24;
    constexpr int NUM_WHITE_KEYS = 35;
    const float totalPianoW = fullAvailW;
    const float keyStep = totalPianoW / static_cast<float>(NUM_WHITE_KEYS);
    constexpr float whiteKeyH = 20.0F;
    const float blackKeyW = std::max(5.0F, std::round(keyStep * 0.58F));
    constexpr float blackKeyH = 12.0F;

    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static constexpr std::array<int, 12> NOTE_TO_WHITE = {
        0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6
    };

    for (int i = 0; i < NUM_WHITE_KEYS; ++i) {
        float x0 = p0.x + std::round(static_cast<float>(i) * keyStep);
        float x1 = (i == NUM_WHITE_KEYS - 1) ? (p0.x + totalPianoW) : (p0.x + std::round(static_cast<float>(i + 1) * keyStep));
        drawList->AddRectFilled(ImVec2(x0, p0.y), ImVec2(x1, p0.y + whiteKeyH), IM_COL32(230, 230, 230, 255));
        drawList->AddRect(ImVec2(x0, p0.y), ImVec2(x1, p0.y + whiteKeyH), IM_COL32(60, 60, 60, 255));
    }

    for (int oct = 0; oct < 5; ++oct) {
        int baseW = oct * 7;
        const std::array<int, 5> blackOffsets = {0, 1, 3, 4, 5};
        for (int b : blackOffsets) {
            float kx = p0.x + std::round((static_cast<float>(baseW + b) * keyStep) + (keyStep * 0.68F));
            ImVec2 kp0(kx, p0.y);
            ImVec2 kp1(kx + blackKeyW, p0.y + blackKeyH);
            drawList->AddRectFilled(kp0, kp1, IM_COL32(35, 35, 35, 255));
        }
    }

    for (int v = 0; v < 3; ++v) {
        int note = activeNotes.at(v);
        if (note < START_NOTE || note >= (START_NOTE + 60)) {
            continue;
        }
        int relNote = note - START_NOTE;
        int oct = relNote / 12;
        int nInOct = relNote % 12;
        int whiteIdx = NOTE_TO_WHITE.at(nInOct);

        if (whiteIdx >= 0) {
            int keyIdx = (oct * 7) + whiteIdx;
            float x0 = p0.x + std::round(static_cast<float>(keyIdx) * keyStep);
            float x1 = (keyIdx == NUM_WHITE_KEYS - 1) ? (p0.x + totalPianoW) : (p0.x + std::round(static_cast<float>(keyIdx + 1) * keyStep));
            drawList->AddRectFilled(ImVec2(x0, p0.y + blackKeyH), ImVec2(x1, p0.y + whiteKeyH), VOICE_COLORS.at(v));
        } else {
            int prevWhite = NOTE_TO_WHITE.at(nInOct - 1);
            int keyIdx = (oct * 7) + prevWhite;
            float kx = p0.x + std::round((static_cast<float>(keyIdx) * keyStep) + (keyStep * 0.68F));
            drawList->AddRectFilled(ImVec2(kx, p0.y), ImVec2(kx + blackKeyW, p0.y + blackKeyH), VOICE_COLORS.at(v));
        }
    }

    ImGui::Dummy(ImVec2(totalPianoW, whiteKeyH));
}

static void DrawCompactSpectrum(const std::array<float, 32>& spectrum,
                                const std::array<std::array<float, 3>, 32>& voiceShares) {
    const float availW = ImGui::GetContentRegionAvail().x;
    constexpr float barH = 14.0F;
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static std::array<float, 32> compactPeakHold{};
    static std::array<float, 32> compactPeakTimers{};
    SIDAudioAnalyzer::UpdatePeakHold(spectrum, compactPeakHold, compactPeakTimers, ImGui::GetIO().DeltaTime);

    drawList->AddRectFilled(p0, ImVec2(p0.x + availW, p0.y + barH), ImColor(Theme::Surface));
    const float barW = (availW - 4.0F) / 32.0F;
    const float maxH = barH - 2.0F;
    for (size_t i = 0; i < 32; ++i) {
        float val = spectrum.at(i);
        float h = (val > 0.01F) ? std::max(1.5F, val * maxH) : 0.0F;
        float bx0 = p0.x + 2.0F + (static_cast<float>(i) * barW);
        float bx1 = bx0 + barW - 1.0F;
        float by1 = p0.y + barH - 1.0F;

        if (h > 0.0F) {
            const auto& shares = voiceShares.at(i);
            float curBottom = by1;
            float topY = by1 - h;
            for (size_t v = 0; v < 3; ++v) {
                float segH = h * shares.at(v);
                if (segH > 0.0F) {
                    float nextY = (v == 2) ? topY : std::max(topY, curBottom - segH);
                    drawList->AddRectFilled(ImVec2(bx0, nextY), ImVec2(bx1, curBottom), VOICE_COLORS.at(v));
                    curBottom = nextY;
                }
            }
        }

        float peakH = std::clamp(compactPeakHold.at(i) * maxH, 0.0F, maxH);
        if (peakH > 1.0F) {
            float peakY = by1 - peakH;
            drawList->AddLine(ImVec2(bx0, peakY), ImVec2(bx1, peakY), IM_COL32(255, 255, 255, 200), 1.0F);
        }
    }
    ImGui::Dummy(ImVec2(availW, barH));
}

static void DrawFullSpectrogramTab(Hardware::SID& sid, const std::array<float, 32>& spectrum,
                                   const std::array<std::array<float, 3>, 32>& voiceShares,
                                   bool should_update) {
    ImGui::PushID("FullSpectrogramTab");
    ImGui::TextUnformatted("Mute/Solo:");
    for (int v = 0; v < 3; ++v) {
        ImGui::SameLine(0.0F, 12.0F);
        ImGui::TextColored(ImColor(VOICE_COLORS.at(v)).Value, "V%d", v + 1);
        ImGui::SameLine(0.0F, 4.0F);
        DrawMuteSoloButtons(sid, v);
    }

    ImGui::Separator();

    ImGui::TextUnformatted("Frequency Spectrum:");
    const float availW = ImGui::GetContentRegionAvail().x;
    const float barHeight = 42.0F;
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static std::array<float, 32> fullPeakHold{};
    static std::array<float, 32> fullPeakTimers{};
    SIDAudioAnalyzer::UpdatePeakHold(spectrum, fullPeakHold, fullPeakTimers, ImGui::GetIO().DeltaTime);

    drawList->AddRectFilled(p0, ImVec2(p0.x + availW, p0.y + barHeight), ImColor(Theme::Surface));
    drawList->AddRect(p0, ImVec2(p0.x + availW, p0.y + barHeight), ImColor(Theme::BorderSubtle));

    const float barWidth = (availW - 4.0F) / 32.0F;
    const float maxBarH = barHeight - 6.0F;
    for (size_t i = 0; i < 32; ++i) {
        float val = spectrum.at(i);
        float h = (val > 0.01F) ? std::max(2.0F, val * maxBarH) : 0.0F;
        float bx0 = p0.x + 2.0F + (static_cast<float>(i) * barWidth);
        float bx1 = bx0 + barWidth - 1.0F;
        float by1 = p0.y + barHeight - 3.0F;

        if (h > 0.0F) {
            const auto& shares = voiceShares.at(i);
            float curBottom = by1;
            float topY = by1 - h;
            for (size_t v = 0; v < 3; ++v) {
                float segH = h * shares.at(v);
                if (segH > 0.0F) {
                    float nextY = (v == 2) ? topY : std::max(topY, curBottom - segH);
                    drawList->AddRectFilled(ImVec2(bx0, nextY), ImVec2(bx1, curBottom), VOICE_COLORS.at(v));
                    curBottom = nextY;
                }
            }
        }

        float peakH = std::clamp(fullPeakHold.at(i) * maxBarH, 0.0F, maxBarH);
        if (peakH > 1.5F) {
            float peakY = by1 - peakH;
            drawList->AddLine(ImVec2(bx0, peakY), ImVec2(bx1, peakY), IM_COL32(255, 255, 255, 210), 1.5F);
        }
    }
    ImGui::Dummy(ImVec2(availW, barHeight + 4.0F));

    ImGui::TextUnformatted("Waterfall Spectrogram (Time vs Freq):");
    struct WaterfallCell {
        float val = 0.0F;
        std::array<float, 3> voiceShares{1.0F / 3.0F, 1.0F / 3.0F, 1.0F / 3.0F};
    };
    constexpr int WATERFALL_COLS = 32;
    constexpr int WATERFALL_ROWS = 28;
    static std::array<std::array<WaterfallCell, WATERFALL_COLS>, WATERFALL_ROWS> waterfallGrid{};
    if (should_update) {
        for (int r = WATERFALL_ROWS - 1; r > 0; --r) {
            waterfallGrid.at(r) = waterfallGrid.at(r - 1);
        }
        for (size_t c = 0; c < WATERFALL_COLS; ++c) {
            waterfallGrid.at(0).at(c) = {spectrum.at(c), voiceShares.at(c)};
        }
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
            const auto& cell = waterfallGrid.at(r).at(c);
            float val = cell.val;
            float cx0 = wp0.x + (static_cast<float>(c) * cellW);
            float cy0 = wp0.y + (static_cast<float>(r) * cellH);
            ImVec2 cp0(cx0 + 0.5F, cy0 + 0.5F);
            ImVec2 cp1(cx0 + cellW - 0.5F, cy0 + cellH - 0.5F);

            ImU32 cellColor = IM_COL32(11, 29, 40, 220);
            if (val > 0.04F) {
                const auto& shares = cell.voiceShares;
                float rMix = (shares.at(0) * 79.0F) + (shares.at(1) * 255.0F) + (shares.at(2) * 240.0F);
                float gMix = (shares.at(0) * 195.0F) + (shares.at(1) * 183.0F) + (shares.at(2) * 98.0F);
                float bMix = (shares.at(0) * 247.0F) + (shares.at(1) * 77.0F) + (shares.at(2) * 146.0F);

                float intensity = std::clamp(val, 0.25F, 1.0F);
                int alpha = static_cast<int>(std::clamp(160.0F + (val * 95.0F), 160.0F, 255.0F));
                cellColor = IM_COL32(
                    static_cast<int>(rMix * intensity),
                    static_cast<int>(gMix * intensity),
                    static_cast<int>(bMix * intensity),
                    alpha
                );
            }
            drawList->AddRectFilled(cp0, cp1, cellColor);
        }
    }
    ImGui::Dummy(ImVec2(availW, totalWaterfallH));
    ImGui::PopID();
}

static bool DrawToggleButton(const char* label, bool active) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_Border, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_Text, ImColor(Theme::LcdPetroleumBg).Value);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BorderSubtle);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::BorderSubtle);
        ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderSubtle);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0F, 1.5F));

    bool clicked = ImGui::Button(label);

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
    return clicked;
}

static void DrawFilterAndRegistersTab(Hardware::SID& sid, bool should_update) {
    (void)should_update;
    ImGui::PushID("FilterAndRegistersTab");

    const uint8_t reg15 = sid.GetRegister(0x15);
    const uint8_t reg16 = sid.GetRegister(0x16);
    const uint8_t reg17 = sid.GetRegister(0x17);
    const uint8_t reg18 = sid.GetRegister(0x18);

    const uint16_t fc = static_cast<uint16_t>((reg15 & 0x07U) | (static_cast<uint16_t>(reg16) << 3U));
    const uint8_t res = (reg17 >> 4U) & 0x0FU;
    const bool f1 = (reg17 & 0x01U) != 0;
    const bool f2 = (reg17 & 0x02U) != 0;
    const bool f3 = (reg17 & 0x04U) != 0;
    const bool mode3Off = (reg18 & 0x80U) != 0;
    const bool modeHP = (reg18 & 0x40U) != 0;
    const bool modeBP = (reg18 & 0x20U) != 0;
    const bool modeLP = (reg18 & 0x10U) != 0;
    const uint8_t volume = reg18 & 0x0FU;

    const auto filterResp = SIDAudioAnalyzer::ComputeFilterResponse(fc, res, sid.GetModel(), modeLP, modeBP, modeHP);
    const double cutoffHz = filterResp.cutoffHz;

    const float availW = ImGui::GetContentRegionAvail().x;
    constexpr float graphH = 70.0F;
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1 = ImVec2(p0.x + availW, p0.y + graphH);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(p0, p1, ImColor(Theme::Surface));
    drawList->AddRect(p0, p1, ImColor(Theme::BorderSubtle));

    constexpr std::array<std::pair<double, const char*>, 3> GRID_FREQS = {{
        {100.0, "100Hz"},
        {1000.0, "1kHz"},
        {10000.0, "10kHz"}
    }};
    for (const auto& [gridF, label] : GRID_FREQS) {
        const double t = std::log10(gridF / 20.0) / 3.0;
        const float gx = p0.x + static_cast<float>(t) * availW;
        drawList->AddLine(ImVec2(gx, p0.y), ImVec2(gx, p1.y), IM_COL32(255, 255, 255, 25), 1.0F);
        drawList->AddText(ImVec2(gx + 2.0F, p0.y + 2.0F), IM_COL32(160, 175, 190, 110), label);
    }

    const float y0dB = p0.y + (18.0F / 42.0F) * graphH;
    drawList->AddLine(ImVec2(p0.x, y0dB), ImVec2(p0.x + availW, y0dB), IM_COL32(255, 255, 255, 30), 1.0F);
    drawList->AddText(ImVec2(p0.x + 3.0F, y0dB - 12.0F), IM_COL32(160, 175, 190, 100), "0 dB");

    constexpr size_t POINT_COUNT = SIDAudioAnalyzer::FILTER_POINTS;
    std::array<ImVec2, POINT_COUNT> curvePoints{};
    for (size_t i = 0; i < POINT_COUNT; ++i) {
        const double t = static_cast<double>(i) / 99.0;
        curvePoints.at(i) = ImVec2(p0.x + static_cast<float>(t) * availW, p0.y + filterResp.yNorms.at(i) * graphH);
    }

    const ImU32 fillCol = ImColor(Theme::AccentPrimary.x, Theme::AccentPrimary.y, Theme::AccentPrimary.z, 0.20F);
    for (size_t i = 0; i < POINT_COUNT - 1; ++i) {
        drawList->AddQuadFilled(ImVec2(curvePoints.at(i).x, p1.y), curvePoints.at(i),
                                curvePoints.at(i + 1), ImVec2(curvePoints.at(i + 1).x, p1.y), fillCol);
    }
    drawList->AddPolyline(curvePoints.data(), static_cast<int>(POINT_COUNT), ImColor(Theme::AccentPrimary), 0, 2.0F);

    const double cutoffT = std::clamp(std::log10(cutoffHz / 20.0) / 3.0, 0.0, 1.0);
    const float fcX = p0.x + static_cast<float>(cutoffT) * availW;
    drawList->AddLine(ImVec2(fcX, p0.y), ImVec2(fcX, p1.y), IM_COL32(255, 90, 90, 220), 1.5F);
    drawList->AddText(ImVec2(fcX + 3.0F, p1.y - 14.0F), IM_COL32(255, 120, 120, 220), "fc");

    ImGui::Dummy(ImVec2(availW, graphH));

    ImGui::Spacing();
    DrawColoredText(Theme::TextSecondary, "Mode:");
    ImGui::SameLine(0.0F, 4.0F);
    if (DrawToggleButton("LP", modeLP)) {
        sid.Write(0x18, static_cast<uint8_t>(reg18 ^ 0x10U));
    }
    ImGui::SameLine(0.0F, 4.0F);
    if (DrawToggleButton("BP", modeBP)) {
        sid.Write(0x18, static_cast<uint8_t>(reg18 ^ 0x20U));
    }
    ImGui::SameLine(0.0F, 4.0F);
    if (DrawToggleButton("HP", modeHP)) {
        sid.Write(0x18, static_cast<uint8_t>(reg18 ^ 0x40U));
    }

    ImGui::SameLine(0.0F, 14.0F);
    DrawColoredText(Theme::TextSecondary, "Route:");
    ImGui::SameLine(0.0F, 4.0F);
    if (DrawToggleButton("F1", f1)) {
        sid.Write(0x17, static_cast<uint8_t>(reg17 ^ 0x01U));
    }
    ImGui::SameLine(0.0F, 4.0F);
    if (DrawToggleButton("F2", f2)) {
        sid.Write(0x17, static_cast<uint8_t>(reg17 ^ 0x02U));
    }
    ImGui::SameLine(0.0F, 4.0F);
    if (DrawToggleButton("F3", f3)) {
        sid.Write(0x17, static_cast<uint8_t>(reg17 ^ 0x04U));
    }
    ImGui::SameLine(0.0F, 4.0F);
    if (DrawToggleButton("3 OFF", mode3Off)) {
        sid.Write(0x18, static_cast<uint8_t>(reg18 ^ 0x80U));
    }

    ImGui::SameLine(0.0F, 14.0F);
    DrawColoredText(Theme::TextSecondary, "Cutoff:");
    ImGui::SameLine(0.0F, 4.0F);
    DrawColoredText(Theme::AccentActive, std::format("${:04X} ({}) ~{:.0f} Hz", fc, fc, cutoffHz));

    ImGui::SameLine(0.0F, 10.0F);
    DrawColoredText(Theme::TextSecondary, "Res:");
    ImGui::SameLine(0.0F, 4.0F);
    DrawColoredText(Theme::AccentActive, std::format("${:X} ({}/15)", res, res));

    ImGui::SameLine(0.0F, 10.0F);
    DrawColoredText(Theme::TextSecondary, "Vol:");
    ImGui::SameLine(0.0F, 4.0F);
    DrawColoredText(Theme::AccentActive, std::format("${:X}", volume));

    ImGui::Separator();
    ImGui::Spacing();

    static bool autoScroll = true;
    ImGui::Checkbox("Auto-scroll", &autoScroll);
    ImGui::SameLine(0.0F, 12.0F);
    if (ImGui::Button("Clear")) {
        sid.ClearTrackerLog();
    }
    ImGui::SameLine(0.0F, 12.0F);

    std::vector<Hardware::SIDTrackerFrame> frames;
    sid.GetTrackerLog(frames);
    DrawColoredText(Theme::TextSecondary, std::format("Frames: {}", frames.size()));

    static constexpr std::array<const char*, 12> TRACKER_NOTE_NAMES = {
        "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
    };

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;

    if (ImGui::BeginTable("SIDTrackerTable", 21, tableFlags, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V1 Note", ImGuiTableColumnFlags_WidthFixed, 44.0F);
        ImGui::TableSetupColumn("V1 Freq", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V1 Wave", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V1 ADSR", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V1 PW", ImGuiTableColumnFlags_WidthFixed, 36.0F);
        ImGui::TableSetupColumn("V2 Note", ImGuiTableColumnFlags_WidthFixed, 44.0F);
        ImGui::TableSetupColumn("V2 Freq", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V2 Wave", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V2 ADSR", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V2 PW", ImGuiTableColumnFlags_WidthFixed, 36.0F);
        ImGui::TableSetupColumn("V3 Note", ImGuiTableColumnFlags_WidthFixed, 44.0F);
        ImGui::TableSetupColumn("V3 Freq", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V3 Wave", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V3 ADSR", ImGuiTableColumnFlags_WidthFixed, 42.0F);
        ImGui::TableSetupColumn("V3 PW", ImGuiTableColumnFlags_WidthFixed, 36.0F);
        ImGui::TableSetupColumn("FCut", ImGuiTableColumnFlags_WidthFixed, 36.0F);
        ImGui::TableSetupColumn("Res", ImGuiTableColumnFlags_WidthFixed, 26.0F);
        ImGui::TableSetupColumn("Route", ImGuiTableColumnFlags_WidthFixed, 36.0F);
        ImGui::TableSetupColumn("Mode", ImGuiTableColumnFlags_WidthFixed, 36.0F);
        ImGui::TableSetupColumn("Vol", ImGuiTableColumnFlags_WidthFixed, 26.0F);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(frames.size()));
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const auto& frame = frames.at(static_cast<size_t>(row));
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                DrawColoredText(Theme::TextSecondary, std::format("{:04X}", frame.frameNumber));

                for (size_t v = 0; v < 3; ++v) {
                    const size_t base = v * 7U;
                    const ImVec4 voiceCol = ImColor(VOICE_COLORS.at(v)).Value;

                    const uint16_t freq = static_cast<uint16_t>(
                        frame.registers.at(base) | (static_cast<uint16_t>(frame.registers.at(base + 1)) << 8U));
                    const bool freqChanged = (frame.writeMask & ((1U << base) | (1U << (base + 1)))) != 0;

                    ImGui::TableNextColumn();
                    if (freqChanged) {
                        const double hz = static_cast<double>(freq) * (1000000.0 / 16777216.0);
                        if (freq > 0 && hz >= 16.0) {
                            const int midiNote = static_cast<int>(std::round(69.0 + (12.0 * std::log2(hz / 440.0))));
                            const int nInOct = ((midiNote % 12) + 12) % 12;
                            const int oct = (midiNote / 12) - 1;
                            DrawColoredText(voiceCol,
                                            std::format("{}{}", TRACKER_NOTE_NAMES.at(static_cast<size_t>(nInOct)), oct));
                        } else {
                            DrawColoredText(voiceCol, "---");
                        }
                    } else {
                        DrawColoredText(Theme::TextSecondary, "....");
                    }

                    ImGui::TableNextColumn();
                    if (freqChanged) {
                        DrawColoredText(voiceCol, std::format("{:04X}", freq));
                    } else {
                        DrawColoredText(Theme::TextSecondary, "....");
                    }

                    ImGui::TableNextColumn();
                    const uint8_t ctrl = frame.registers.at(base + 4);
                    if ((frame.writeMask & (1U << (base + 4))) != 0) {
                        DrawColoredText(voiceCol, std::format("{:02X} {}", ctrl, (ctrl & 0x01) != 0 ? "G" : "."));
                    } else {
                        DrawColoredText(Theme::TextSecondary, ".. .");
                    }

                    ImGui::TableNextColumn();
                    if ((frame.writeMask & ((1U << (base + 5)) | (1U << (base + 6)))) != 0) {
                        DrawColoredText(voiceCol, std::format("{:02X}{:02X}", frame.registers.at(base + 5),
                                                              frame.registers.at(base + 6)));
                    } else {
                        DrawColoredText(Theme::TextSecondary, "....");
                    }

                    ImGui::TableNextColumn();
                    if ((frame.writeMask & ((1U << (base + 2)) | (1U << (base + 3)))) != 0) {
                        const uint16_t pw = static_cast<uint16_t>(
                            frame.registers.at(base + 2) | ((frame.registers.at(base + 3) & 0x0FU) << 8U));
                        DrawColoredText(voiceCol, std::format("{:03X}", pw));
                    } else {
                        DrawColoredText(Theme::TextSecondary, "...");
                    }
                }

                const ImVec4 filterCol = Theme::AccentActive;

                ImGui::TableNextColumn();
                if ((frame.writeMask & ((1U << 0x15) | (1U << 0x16))) != 0) {
                    const uint16_t rowFc = static_cast<uint16_t>(
                        (frame.registers.at(0x15) & 0x07U) | (static_cast<uint16_t>(frame.registers.at(0x16)) << 3U));
                    DrawColoredText(filterCol, std::format("{:03X}", rowFc));
                } else {
                    DrawColoredText(Theme::TextSecondary, "...");
                }

                ImGui::TableNextColumn();
                if ((frame.writeMask & (1U << 0x17)) != 0) {
                    const uint8_t rowRes = (frame.registers.at(0x17) >> 4U) & 0x0FU;
                    DrawColoredText(filterCol, std::format("{:X}", rowRes));
                } else {
                    DrawColoredText(Theme::TextSecondary, ".");
                }

                ImGui::TableNextColumn();
                if ((frame.writeMask & (1U << 0x17)) != 0) {
                    const uint8_t r17 = frame.registers.at(0x17);
                    std::string route;
                    route.reserve(3);
                    route += ((r17 & 0x01U) != 0) ? '1' : '-';
                    route += ((r17 & 0x02U) != 0) ? '2' : '-';
                    route += ((r17 & 0x04U) != 0) ? '3' : '-';
                    DrawColoredText(filterCol, route);
                } else {
                    DrawColoredText(Theme::TextSecondary, "...");
                }

                ImGui::TableNextColumn();
                if ((frame.writeMask & (1U << 0x18)) != 0) {
                    const uint8_t r18 = frame.registers.at(0x18);
                    std::string mode;
                    mode.reserve(4);
                    mode += ((r18 & 0x10U) != 0) ? 'L' : '-';
                    mode += ((r18 & 0x20U) != 0) ? 'B' : '-';
                    mode += ((r18 & 0x40U) != 0) ? 'H' : '-';
                    mode += ((r18 & 0x80U) != 0) ? '3' : '-';
                    DrawColoredText(filterCol, mode);
                } else {
                    DrawColoredText(Theme::TextSecondary, "....");
                }

                ImGui::TableNextColumn();
                if ((frame.writeMask & (1U << 0x18)) != 0) {
                    const uint8_t rowVol = frame.registers.at(0x18) & 0x0FU;
                    DrawColoredText(filterCol, std::format("{:X}", rowVol));
                } else {
                    DrawColoredText(Theme::TextSecondary, ".");
                }
            }
        }

        static size_t lastFrameCount = 0;
        if (autoScroll && !frames.empty() &&
            (frames.size() > lastFrameCount || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
            ImGui::SetScrollHereY(1.0F);
        }
        lastFrameCount = frames.size();

        ImGui::EndTable();
    }

    ImGui::PopID();
}

static void DrawSIDViewerContent(AppState& state, double sid_time, bool should_update) {
    std::array<float, 32> spectrum{};
    std::array<std::array<float, 3>, 32> voiceShares{};
    static SIDAudioAnalyzer audioAnalyzer;
    audioAnalyzer.ProcessFFT(state.emulator.GetSID(), spectrum, voiceShares, should_update);

    if (ImGui::BeginTabBar("SIDViewerTabs")) {
        if (ImGui::BeginTabItem("Oscillators")) {
            for (int voiceIndex = 0; voiceIndex < 3; ++voiceIndex) {
                ImGui::PushID(voiceIndex);
                if (voiceIndex > 0) {
                    ImGui::Separator();
                }

                DrawVoiceWaveform(state.emulator.GetSID(), voiceIndex, sid_time, should_update, state.emulation.sidPhaseTrigger);
                ImGui::SameLine(0.0F, 28.0F);
                DrawVoiceParameters(state.emulator.GetSID(), voiceIndex, state.emulation.sidADSRWarning);

                ImGui::PopID();
            }

            ImGui::Separator();
            DrawPianoRoll(state.emulator.GetSID());
            ImGui::Spacing();
            DrawCompactSpectrum(spectrum, voiceShares);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("FFT Spectrogram")) {
            DrawFullSpectrogramTab(state.emulator.GetSID(), spectrum, voiceShares, should_update);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Filter & Registers")) {
            DrawFilterAndRegistersTab(state.emulator.GetSID(), should_update);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void DrawSIDViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                         ImGuiWindowFlags window_flags) {
    float leftWidth = work_size.x * 0.47F;
    float sidWidth = work_size.x - leftWidth;
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + leftWidth, work_pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(sidWidth, top_section_height), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0F * state.render.uiScale, 2.0F * state.render.uiScale));
    ImGui::Begin("SID Viewer", nullptr, window_flags | ImGuiWindowFlags_NoMove);

    state.render.sidWindowPos[0] = ImGui::GetWindowPos().x;
    state.render.sidWindowPos[1] = ImGui::GetWindowPos().y;
    state.render.sidWindowSize[0] = ImGui::GetWindowSize().x;
    state.render.sidWindowSize[1] = ImGui::GetWindowSize().y;

    static double sid_time = 0.0;
    const bool midiActive = (state.ide.open && state.ide.mode == Control::IDEMode::MIDI && GUI::IsMIDIAudioActive());
    if ((state.emulator.IsRunning() && !state.emulator.IsPaused()) || midiActive) {
        sid_time += ImGui::GetIO().DeltaTime;
    }

    bool should_update = (state.emulator.IsRunning() && !state.emulator.IsPaused()) || midiActive;
    DrawSIDViewerContent(state, sid_time, should_update);

    ImGui::End();
    ImGui::PopStyleVar();
}

}  // namespace GUI
