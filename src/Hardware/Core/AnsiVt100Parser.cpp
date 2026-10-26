#include "Hardware/Core/AnsiVt100Parser.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

#include "Hardware/Core/TerminalBuffer.h"

namespace Console {

static constexpr std::array<uint32_t, 16> ANSI_PALETTE = {
    0xFF000000, 0xFF2B31CD, 0xFF79BC0D, 0xFF10E5E5, 0xFFC87224, 0xFFBC3FBC, 0xFFCDA811, 0xFFE5E5E5,
    0xFF666666, 0xFF4C4CF1, 0xFF8BD123, 0xFF43F5F5, 0xFFEA8E3B, 0xFFD670D6, 0xFFDBB829, 0xFFFFFFFF};

static constexpr std::array<uint8_t, 6> CUBE_STEPS = {0, 95, 135, 175, 215, 255};

static uint32_t GetColor256(int index) {
    if (index < 0 || index > 255) {
        return DEFAULT_FG;
    }
    if (index < 16) {
        return ANSI_PALETTE[index];
    }
    if (index < 232) {
        int cubeIdx = index - 16;
        int rIdx = cubeIdx / 36;
        int gIdx = (cubeIdx / 6) % 6;
        int bIdx = cubeIdx % 6;
        uint32_t r = CUBE_STEPS[rIdx];
        uint32_t g = CUBE_STEPS[gIdx];
        uint32_t b = CUBE_STEPS[bIdx];
        return 0xFF000000 | (b << 16) | (g << 8) | r;
    }
    auto gray = static_cast<uint32_t>(8 + 10 * (index - 232));
    return 0xFF000000 | (gray << 16) | (gray << 8) | gray;
}

static bool ParseColor(std::string_view str, uint32_t& outColor) {
    if (str.starts_with('#')) {
        str.remove_prefix(1);
    } else if (str.starts_with("rgb:")) {
        str.remove_prefix(4);
    }
    if (str.length() == 6) {
        auto fromHex = [](char c) -> int {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
            }
            if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
            }
            return -1;
        };
        int r1 = fromHex(str[0]);
        int r2 = fromHex(str[1]);
        int g1 = fromHex(str[2]);
        int g2 = fromHex(str[3]);
        int b1 = fromHex(str[4]);
        int b2 = fromHex(str[5]);
        if (r1 >= 0 && r2 >= 0 && g1 >= 0 && g2 >= 0 && b1 >= 0 && b2 >= 0) {
            auto r = static_cast<uint32_t>((r1 << 4) | r2);
            auto g = static_cast<uint32_t>((g1 << 4) | g2);
            auto b = static_cast<uint32_t>((b1 << 4) | b2);
            outColor = 0xFF000000 | (b << 16) | (g << 8) | r;
            return true;
        }
    }
    bool allDigits = !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
    if (allDigits) {
        int idx = 0;
        for (char c : str) {
            idx = idx * 10 + (c - '0');
            if (idx > 255) {
                return false;
            }
        }
        outColor = GetColor256(idx);
        return true;
    }
    return false;
}

static uint32_t MapDecGraphics(char chr) {
    switch (chr) {
        case '`': return 0x25C6;
        case 'a': return 0x2592;
        case 'f': return 0x00B0;
        case 'g': return 0x00B1;
        case 'h': return 0x2424;
        case 'i': return 0x240B;
        case 'j': return 0x2518;
        case 'k': return 0x2510;
        case 'l': return 0x250C;
        case 'm': return 0x2514;
        case 'n': return 0x253C;
        case 'o': return 0x23BA;
        case 'p': return 0x23BB;
        case 'q': return 0x2500;
        case 'r': return 0x23BC;
        case 's': return 0x23BD;
        case 't': return 0x251C;
        case 'u': return 0x2524;
        case 'v': return 0x2534;
        case 'w': return 0x252C;
        case 'x': return 0x2502;
        case 'y': return 0x2264;
        case 'z': return 0x2265;
        case '{': return 0x03C0;
        case '|': return 0x2260;
        case '}': return 0x00A3;
        case '~': return 0x00B7;
        default: return static_cast<uint32_t>(chr);
    }
}

void AnsiVt100Parser::Reset() {
    parserState = ParserState::Ground;
    oscBuffer.clear();
    csiParams.clear();
    currentParam = 0;
    hasCurrentParam = false;
    isPrivateMode = false;
    utf8Remaining = 0;
    decSpecialGraphics = false;
}

void AnsiVt100Parser::SetResponseCallback(std::function<void(char)> callback) {
    responseCallback = std::move(callback);
}

void AnsiVt100Parser::ExecuteOsc(TerminalBuffer& buffer) {
    auto sep = oscBuffer.find(';');
    if (sep == std::string::npos) {
        if (oscBuffer == "110") {
            buffer.defaultFg = DEFAULT_FG;
            buffer.currentFg = buffer.defaultFg;
        } else if (oscBuffer == "111") {
            buffer.defaultBg = DEFAULT_BG;
            buffer.currentBg = buffer.defaultBg;
        }
        return;
    }
    std::string_view cmd = std::string_view(oscBuffer).substr(0, sep);
    std::string_view val = std::string_view(oscBuffer).substr(sep + 1);
    uint32_t col = 0;
    if (cmd == "10" && ParseColor(val, col)) {
        buffer.defaultFg = col;
        buffer.currentFg = col;
    } else if (cmd == "11" && ParseColor(val, col)) {
        buffer.defaultBg = col;
        buffer.currentBg = col;
    }
}

void AnsiVt100Parser::HandleSGR(TerminalBuffer& buffer) {
    if (csiParams.empty()) {
        csiParams.push_back(0);
    }
    for (size_t i = 0; i < csiParams.size(); ++i) {
        int param = csiParams[i];
        if (param == 0) {
            buffer.currentFg = buffer.defaultFg;
            buffer.currentBg = buffer.defaultBg;
            buffer.currentAttrs = 0;
        } else if (param == 1) {
            buffer.currentAttrs |= ATTR_BOLD;
        } else if (param == 4) {
            buffer.currentAttrs |= ATTR_UNDERLINE;
        } else if (param == 7) {
            buffer.currentAttrs |= ATTR_INVERSE;
        } else if (param == 22) {
            buffer.currentAttrs &= ~ATTR_BOLD;
        } else if (param == 24) {
            buffer.currentAttrs &= ~ATTR_UNDERLINE;
        } else if (param == 27) {
            buffer.currentAttrs &= ~ATTR_INVERSE;
        } else if (param >= 30 && param <= 37) {
            buffer.currentFg = ANSI_PALETTE[param - 30];
        } else if (param == 38 || param == 48) {
            bool isFg = (param == 38);
            if (i + 1 < csiParams.size()) {
                if (csiParams[i + 1] == 5 && i + 2 < csiParams.size()) {
                    uint32_t col = GetColor256(csiParams[i + 2]);
                    if (isFg) {
                        buffer.currentFg = col;
                    } else {
                        buffer.currentBg = col;
                    }
                    i += 2;
                } else if (csiParams[i + 1] == 2 && i + 4 < csiParams.size()) {
                    auto r = static_cast<uint32_t>(std::clamp(csiParams[i + 2], 0, 255));
                    auto g = static_cast<uint32_t>(std::clamp(csiParams[i + 3], 0, 255));
                    auto b = static_cast<uint32_t>(std::clamp(csiParams[i + 4], 0, 255));
                    uint32_t col = 0xFF000000 | (b << 16) | (g << 8) | r;
                    if (isFg) {
                        buffer.currentFg = col;
                    } else {
                        buffer.currentBg = col;
                    }
                    i += 4;
                }
            }
        } else if (param == 39) {
            buffer.currentFg = buffer.defaultFg;
        } else if (param >= 40 && param <= 47) {
            buffer.currentBg = ANSI_PALETTE[param - 40];
        } else if (param == 49) {
            buffer.currentBg = buffer.defaultBg;
        } else if (param >= 90 && param <= 97) {
            buffer.currentFg = ANSI_PALETTE[param - 90 + 8];
        } else if (param >= 100 && param <= 107) {
            buffer.currentBg = ANSI_PALETTE[param - 100 + 8];
        }
    }
}

void AnsiVt100Parser::ExecuteCsi(char cmd, TerminalBuffer& buffer) {
    if (hasCurrentParam || !csiParams.empty()) {
        csiParams.push_back(currentParam);
    }

    switch (cmd) {
        case 'H':
        case 'f': {
            int row = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
            int col = (csiParams.size() > 1 && csiParams[1] > 0) ? csiParams[1] : 1;
            buffer.cursorY = std::clamp(row - 1, 0, TERMINAL_ROWS - 1);
            buffer.cursorX = std::clamp(col - 1, 0, TERMINAL_COLS - 1);
            break;
        }
        case 'A': {
            int count = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
            buffer.cursorY = std::max(0, buffer.cursorY - count);
            break;
        }
        case 'B': {
            int count = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
            buffer.cursorY = std::min(TERMINAL_ROWS - 1, buffer.cursorY + count);
            break;
        }
        case 'C': {
            int count = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
            buffer.cursorX = std::min(TERMINAL_COLS - 1, buffer.cursorX + count);
            break;
        }
        case 'D': {
            int count = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
            buffer.cursorX = std::max(0, buffer.cursorX - count);
            break;
        }
        case 'J': {
            int mode = !csiParams.empty() ? csiParams[0] : 0;
            buffer.ClearDisplay(mode);
            break;
        }
        case 'K': {
            int mode = !csiParams.empty() ? csiParams[0] : 0;
            buffer.ClearLine(mode);
            break;
        }
        case 'm': {
            HandleSGR(buffer);
            break;
        }
        case 'h': {
            if (isPrivateMode) {
                for (int param : csiParams) {
                    if (param == 25) {
                        buffer.cursorVisible = true;
                    } else if (param == 1049) {
                        if (!buffer.useAltScreen) {
                            buffer.altSavedCursorX = buffer.cursorX;
                            buffer.altSavedCursorY = buffer.cursorY;
                        }
                        buffer.useAltScreen = true;
                        for (auto& row : buffer.altScreen) {
                            row.assign(TERMINAL_COLS, Cell{' ', DEFAULT_FG, DEFAULT_BG, 0});
                        }
                        buffer.cursorX = 0;
                        buffer.cursorY = 0;
                    } else if (param == 47) {
                        buffer.useAltScreen = true;
                    }
                }
            }
            break;
        }
        case 'l': {
            if (isPrivateMode) {
                for (int param : csiParams) {
                    if (param == 25) {
                        buffer.cursorVisible = false;
                    } else if (param == 1049) {
                        if (buffer.useAltScreen) {
                            buffer.useAltScreen = false;
                            buffer.cursorX = buffer.altSavedCursorX;
                            buffer.cursorY = buffer.altSavedCursorY;
                        }
                    } else if (param == 47) {
                        buffer.useAltScreen = false;
                    }
                }
            }
            break;
        }
        case 'n': {
            if (!csiParams.empty() && csiParams[0] == 6 && responseCallback) {
                std::string resp = "\x1b[" + std::to_string(buffer.cursorY + 1) + ";" + std::to_string(buffer.cursorX + 1) + "R";
                for (char ch : resp) {
                    responseCallback(ch);
                }
            }
            break;
        }
        case 's': {
            buffer.savedCursorX = buffer.cursorX;
            buffer.savedCursorY = buffer.cursorY;
            break;
        }
        case 'u': {
            buffer.cursorX = buffer.savedCursorX;
            buffer.cursorY = buffer.savedCursorY;
            break;
        }
        default:
            break;
    }
}

void AnsiVt100Parser::ProcessChar(char character, TerminalBuffer& buffer) {
    auto byteVal = static_cast<uint8_t>(character);

    if (character == 0x1B) {
        utf8Remaining = 0;
        parserState = ParserState::Escape;
        return;
    }
    if (character == 0x18 || character == 0x1A) {
        utf8Remaining = 0;
        parserState = ParserState::Ground;
        return;
    }

    if (parserState == ParserState::Ground) {
        if (utf8Remaining > 0) {
            if ((byteVal & 0xC0) == 0x80) {
                utf8Codepoint = (utf8Codepoint << 6) | (byteVal & 0x3F);
                utf8Remaining--;
                if (utf8Remaining == 0) {
                    buffer.PutChar(utf8Codepoint);
                }
            } else {
                utf8Remaining = 0;
            }
            return;
        }

        if ((byteVal & 0x80) != 0) {
            if ((byteVal & 0xE0) == 0xC0) {
                utf8Codepoint = byteVal & 0x1F;
                utf8Remaining = 1;
            } else if ((byteVal & 0xF0) == 0xE0) {
                utf8Codepoint = byteVal & 0x0F;
                utf8Remaining = 2;
            } else if ((byteVal & 0xF8) == 0xF0) {
                utf8Codepoint = byteVal & 0x07;
                utf8Remaining = 3;
            }
            return;
        }
        if (character == '\r') {
            buffer.cursorX = 0;
            return;
        }
        if (character == '\n') {
            buffer.cursorY++;
            if (buffer.cursorY >= TERMINAL_ROWS) {
                buffer.ScrollUp();
                buffer.cursorY = TERMINAL_ROWS - 1;
            }
            return;
        }
        if (character == 0x08 || character == 0x7F) {
            if (buffer.cursorX > 0) {
                buffer.cursorX--;
            }
            return;
        }
        if (character == '\t') {
            buffer.cursorX = (buffer.cursorX + 8) & ~7;
            if (buffer.cursorX >= TERMINAL_COLS) {
                buffer.cursorX = TERMINAL_COLS - 1;
            }
            return;
        }
        if (character >= 32) {
            uint32_t cp = static_cast<uint32_t>(byteVal);
            if (decSpecialGraphics) {
                cp = MapDecGraphics(character);
            }
            buffer.PutChar(cp);
        }
    } else if (parserState == ParserState::Escape) {
        if (character == '[') {
            parserState = ParserState::Csi;
            csiParams.clear();
            currentParam = 0;
            hasCurrentParam = false;
            isPrivateMode = false;
        } else if (character == ']') {
            parserState = ParserState::Osc;
            oscBuffer.clear();
        } else if (character == '(') {
            parserState = ParserState::CharsetG0;
        } else if (character == ')') {
            parserState = ParserState::CharsetG1;
        } else if (character == '7') {
            buffer.savedCursorX = buffer.cursorX;
            buffer.savedCursorY = buffer.cursorY;
            buffer.savedFg = buffer.currentFg;
            buffer.savedBg = buffer.currentBg;
            buffer.savedAttrs = buffer.currentAttrs;
            parserState = ParserState::Ground;
        } else if (character == '8') {
            buffer.cursorX = buffer.savedCursorX;
            buffer.cursorY = buffer.savedCursorY;
            buffer.currentFg = buffer.savedFg;
            buffer.currentBg = buffer.savedBg;
            buffer.currentAttrs = buffer.savedAttrs;
            parserState = ParserState::Ground;
        } else if (character == 'c') {
            buffer.ResetBuffer();
            parserState = ParserState::Ground;
        } else {
            parserState = ParserState::Ground;
        }
    } else if (parserState == ParserState::Osc) {
        if (character == 0x07) {
            ExecuteOsc(buffer);
            parserState = ParserState::Ground;
        } else if (character == 0x1B) {
            ExecuteOsc(buffer);
            parserState = ParserState::Escape;
        } else if (oscBuffer.size() < 128) {
            oscBuffer.push_back(character);
        } else {
            parserState = ParserState::Ground;
        }
    } else if (parserState == ParserState::CharsetG0) {
        decSpecialGraphics = (character == '0');
        parserState = ParserState::Ground;
    } else if (parserState == ParserState::CharsetG1) {
        parserState = ParserState::Ground;
    } else if (parserState == ParserState::Csi) {
        if (character >= '0' && character <= '9') {
            if (currentParam < 10000) {
                currentParam = currentParam * 10 + (character - '0');
            }
            hasCurrentParam = true;
        } else if (character == ';') {
            csiParams.push_back(currentParam);
            currentParam = 0;
            hasCurrentParam = false;
        } else if (character == '?') {
            isPrivateMode = true;
        } else {
            ExecuteCsi(character, buffer);
            parserState = ParserState::Ground;
        }
    }
}

}  // namespace Console
