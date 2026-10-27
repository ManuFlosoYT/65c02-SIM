#pragma once

#include <imgui.h>

#include <array>
#include <cstdint>
#include <string>

namespace GUI {

class TerminalInputMapper {
public:
    static inline bool MapNavigationKey(ImGuiKey key, bool shift, [[maybe_unused]] bool ctrl, [[maybe_unused]] bool alt,
                                        std::string& outSeq) {
        if (shift && key == ImGuiKey_Tab) {
            outSeq = "\x1b[Z";
            return true;
        }

        switch (key) {
            case ImGuiKey_UpArrow:    outSeq = "\x1b[A"; return true;
            case ImGuiKey_DownArrow:  outSeq = "\x1b[B"; return true;
            case ImGuiKey_RightArrow: outSeq = "\x1b[C"; return true;
            case ImGuiKey_LeftArrow:  outSeq = "\x1b[D"; return true;
            case ImGuiKey_Home:       outSeq = "\x1b[H"; return true;
            case ImGuiKey_End:        outSeq = "\x1b[F"; return true;
            case ImGuiKey_Insert:     outSeq = "\x1b[2~"; return true;
            case ImGuiKey_Delete:     outSeq = "\x1b[3~"; return true;
            case ImGuiKey_PageUp:     outSeq = "\x1b[5~"; return true;
            case ImGuiKey_PageDown:   outSeq = "\x1b[6~"; return true;
            default:                  return false;
        }
    }

    static inline bool MapCtrlKey(ImGuiKey key, std::string& outSeq) {
        if (key >= ImGuiKey_A && key <= ImGuiKey_Z) {
            outSeq.assign(1, static_cast<char>(1 + (static_cast<int>(key) - static_cast<int>(ImGuiKey_A))));
            return true;
        }
        return false;
    }

    static inline size_t CodepointToUtf8(uint32_t cp, std::array<char, 5>& out) {
        out.fill(0);
        if (cp < 0x80) {
            out[0] = static_cast<char>(cp);
            return 1;
        }
        if (cp < 0x800) {
            out[0] = static_cast<char>(0xC0 | (cp >> 6));
            out[1] = static_cast<char>(0x80 | (cp & 0x3F));
            return 2;
        }
        if (cp < 0x10000) {
            out[0] = static_cast<char>(0xE0 | (cp >> 12));
            out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[2] = static_cast<char>(0x80 | (cp & 0x3F));
            return 3;
        }
        if (cp < 0x110000) {
            out[0] = static_cast<char>(0xF0 | (cp >> 18));
            out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[3] = static_cast<char>(0x80 | (cp & 0x3F));
            return 4;
        }
        return 0;
    }
};

}  // namespace GUI
