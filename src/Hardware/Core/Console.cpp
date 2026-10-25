#include "Hardware/Core/Console.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>

#include "Hardware/Core/ISerializable.h"

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
        case '`':
            return 0x25C6;
        case 'a':
            return 0x2592;
        case 'f':
            return 0x00B0;
        case 'g':
            return 0x00B1;
        case 'h':
            return 0x2424;
        case 'i':
            return 0x240B;
        case 'j':
            return 0x2518;
        case 'k':
            return 0x2510;
        case 'l':
            return 0x250C;
        case 'm':
            return 0x2514;
        case 'n':
            return 0x253C;
        case 'o':
            return 0x23BA;
        case 'p':
            return 0x23BB;
        case 'q':
            return 0x2500;
        case 'r':
            return 0x23BC;
        case 's':
            return 0x23BD;
        case 't':
            return 0x251C;
        case 'u':
            return 0x2524;
        case 'v':
            return 0x2534;
        case 'w':
            return 0x252C;
        case 'x':
            return 0x2502;
        case 'y':
            return 0x2264;
        case 'z':
            return 0x2265;
        case '{':
            return 0x03C0;
        case '|':
            return 0x2260;
        case '}':
            return 0x00A3;
        case '~':
            return 0x00B7;
        default:
            return static_cast<uint32_t>(chr);
    }
}

static void AppendUtf8(std::string& out, uint32_t cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back('?');
    }
}

enum class ParserState { Ground, Escape, Csi, CharsetG0, CharsetG1, Osc };

struct ConsoleState {
    std::vector<std::vector<Cell>> screen;
    std::vector<std::vector<Cell>> altScreen;
    bool useAltScreen = false;
    std::vector<std::string> scrollbackLines;

    int cursorX = 0;
    int cursorY = 0;
    int savedCursorX = 0;
    int savedCursorY = 0;
    int altSavedCursorX = 0;
    int altSavedCursorY = 0;
    bool cursorVisible = true;

    uint32_t currentFg = DEFAULT_FG;
    uint32_t currentBg = DEFAULT_BG;
    uint32_t defaultFg = DEFAULT_FG;
    uint32_t defaultBg = DEFAULT_BG;
    uint8_t currentAttrs = 0;
    uint32_t savedFg = DEFAULT_FG;
    uint32_t savedBg = DEFAULT_BG;
    uint8_t savedAttrs = 0;

    bool decSpecialGraphics = false;
    std::function<void(char)> responseCallback;

    ParserState parserState = ParserState::Ground;
    std::string oscBuffer;
    std::vector<int> csiParams;
    int currentParam = 0;
    bool hasCurrentParam = false;
    bool isPrivateMode = false;

    uint32_t utf8Codepoint = 0;
    int utf8Remaining = 0;

    std::vector<std::string> cachedLines;
    std::string cachedCurrentLine;

    ConsoleState() { ResetBuffer(); }

    void ResetBuffer() {
        screen.assign(TERMINAL_ROWS,
                      std::vector<Cell>(TERMINAL_COLS,
                                        Cell{.codepoint = ' ', .fg = defaultFg, .bg = defaultBg, .attributes = 0}));
        altScreen.assign(TERMINAL_ROWS,
                         std::vector<Cell>(TERMINAL_COLS,
                                           Cell{.codepoint = ' ', .fg = defaultFg, .bg = defaultBg, .attributes = 0}));
        useAltScreen = false;
        scrollbackLines.clear();
        cursorX = 0;
        cursorY = 0;
        savedCursorX = 0;
        savedCursorY = 0;
        altSavedCursorX = 0;
        altSavedCursorY = 0;
        cursorVisible = true;
        currentFg = defaultFg;
        currentBg = defaultBg;
        currentAttrs = 0;
        decSpecialGraphics = false;
        parserState = ParserState::Ground;
        utf8Remaining = 0;
        oscBuffer.clear();
    }

    void ExecuteOsc() {
        auto sep = oscBuffer.find(';');
        if (sep == std::string::npos) {
            if (oscBuffer == "110") {
                defaultFg = DEFAULT_FG;
                currentFg = defaultFg;
            } else if (oscBuffer == "111") {
                defaultBg = DEFAULT_BG;
                currentBg = defaultBg;
            }
            return;
        }
        std::string_view cmd = std::string_view(oscBuffer).substr(0, sep);
        std::string_view val = std::string_view(oscBuffer).substr(sep + 1);
        uint32_t col = 0;
        if (cmd == "10" && ParseColor(val, col)) {
            defaultFg = col;
            currentFg = col;
        } else if (cmd == "11" && ParseColor(val, col)) {
            defaultBg = col;
            currentBg = col;
        }
    }

    [[nodiscard]] std::vector<std::vector<Cell>>& ActiveScreen() { return useAltScreen ? altScreen : screen; }

    [[nodiscard]] const std::vector<std::vector<Cell>>& ActiveScreen() const {
        return useAltScreen ? altScreen : screen;
    }

    void ScrollUp() {
        auto& grid = ActiveScreen();
        if (!useAltScreen) {
            std::string line;
            line.reserve(TERMINAL_COLS);
            for (int col = 0; col < TERMINAL_COLS; ++col) {
                AppendUtf8(line, grid[0][col].codepoint);
            }
            while (!line.empty() && line.back() == ' ') {
                line.pop_back();
            }
            scrollbackLines.push_back(line);
            if (scrollbackLines.size() > CONSOLE_MAX_LINES) {
                scrollbackLines.erase(scrollbackLines.begin());
            }
        }
        for (int row = 0; row < TERMINAL_ROWS - 1; ++row) {
            grid[row] = grid[row + 1];
        }
        grid[TERMINAL_ROWS - 1].assign(TERMINAL_COLS, Cell{' ', currentFg, currentBg, 0});
    }

    void PutChar(uint32_t codepoint) {
        if (cursorX >= TERMINAL_COLS) {
            cursorX = 0;
            cursorY++;
            if (cursorY >= TERMINAL_ROWS) {
                ScrollUp();
                cursorY = TERMINAL_ROWS - 1;
            }
        }
        auto& grid = ActiveScreen();
        grid[cursorY][cursorX] = Cell{codepoint, currentFg, currentBg, currentAttrs};
        cursorX++;
    }

    void ClearDisplay(int mode) {
        auto& grid = ActiveScreen();
        if (mode == 2 || mode == 3) {
            for (auto& row : grid) {
                row.assign(TERMINAL_COLS, Cell{' ', currentFg, currentBg, 0});
            }
        } else if (mode == 0) {
            for (int col = cursorX; col < TERMINAL_COLS; ++col) {
                grid[cursorY][col] = Cell{' ', currentFg, currentBg, 0};
            }
            for (int row = cursorY + 1; row < TERMINAL_ROWS; ++row) {
                grid[row].assign(TERMINAL_COLS, Cell{' ', currentFg, currentBg, 0});
            }
        } else if (mode == 1) {
            for (int row = 0; row < cursorY; ++row) {
                grid[row].assign(TERMINAL_COLS, Cell{' ', currentFg, currentBg, 0});
            }
            for (int col = 0; col <= cursorX && col < TERMINAL_COLS; ++col) {
                grid[cursorY][col] = Cell{' ', currentFg, currentBg, 0};
            }
        }
    }

    void ClearLine(int mode) {
        auto& grid = ActiveScreen();
        if (mode == 2) {
            grid[cursorY].assign(TERMINAL_COLS, Cell{' ', currentFg, currentBg, 0});
        } else if (mode == 0) {
            for (int col = cursorX; col < TERMINAL_COLS; ++col) {
                grid[cursorY][col] = Cell{' ', currentFg, currentBg, 0};
            }
        } else if (mode == 1) {
            for (int col = 0; col <= cursorX && col < TERMINAL_COLS; ++col) {
                grid[cursorY][col] = Cell{' ', currentFg, currentBg, 0};
            }
        }
    }

    void HandleSGR() {
        if (csiParams.empty()) {
            csiParams.push_back(0);
        }
        for (size_t i = 0; i < csiParams.size(); ++i) {
            int param = csiParams[i];
            if (param == 0) {
                currentFg = defaultFg;
                currentBg = defaultBg;
                currentAttrs = 0;
            } else if (param == 1) {
                currentAttrs |= ATTR_BOLD;
            } else if (param == 4) {
                currentAttrs |= ATTR_UNDERLINE;
            } else if (param == 7) {
                currentAttrs |= ATTR_INVERSE;
            } else if (param == 22) {
                currentAttrs &= ~ATTR_BOLD;
            } else if (param == 24) {
                currentAttrs &= ~ATTR_UNDERLINE;
            } else if (param == 27) {
                currentAttrs &= ~ATTR_INVERSE;
            } else if (param >= 30 && param <= 37) {
                currentFg = ANSI_PALETTE[param - 30];
            } else if (param == 38 || param == 48) {
                bool isFg = (param == 38);
                if (i + 1 < csiParams.size()) {
                    if (csiParams[i + 1] == 5 && i + 2 < csiParams.size()) {
                        uint32_t col = GetColor256(csiParams[i + 2]);
                        if (isFg) {
                            currentFg = col;
                        } else {
                            currentBg = col;
                        }
                        i += 2;
                    } else if (csiParams[i + 1] == 2 && i + 4 < csiParams.size()) {
                        auto r = static_cast<uint32_t>(std::clamp(csiParams[i + 2], 0, 255));
                        auto g = static_cast<uint32_t>(std::clamp(csiParams[i + 3], 0, 255));
                        auto b = static_cast<uint32_t>(std::clamp(csiParams[i + 4], 0, 255));
                        uint32_t col = 0xFF000000 | (b << 16) | (g << 8) | r;
                        if (isFg) {
                            currentFg = col;
                        } else {
                            currentBg = col;
                        }
                        i += 4;
                    }
                }
            } else if (param == 39) {
                currentFg = defaultFg;
            } else if (param >= 40 && param <= 47) {
                currentBg = ANSI_PALETTE[param - 40];
            } else if (param == 49) {
                currentBg = defaultBg;
            } else if (param >= 90 && param <= 97) {
                currentFg = ANSI_PALETTE[param - 90 + 8];
            } else if (param >= 100 && param <= 107) {
                currentBg = ANSI_PALETTE[param - 100 + 8];
            }
        }
    }

    void ExecuteCsi(char cmd) {
        if (hasCurrentParam || !csiParams.empty()) {
            csiParams.push_back(currentParam);
        }

        switch (cmd) {
            case 'H':
            case 'f': {
                int row = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
                int col = (csiParams.size() > 1 && csiParams[1] > 0) ? csiParams[1] : 1;
                cursorY = std::clamp(row - 1, 0, TERMINAL_ROWS - 1);
                cursorX = std::clamp(col - 1, 0, TERMINAL_COLS - 1);
                break;
            }
            case 'A': {
                int count = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
                cursorY = std::max(0, cursorY - count);
                break;
            }
            case 'B': {
                int count = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
                cursorY = std::min(TERMINAL_ROWS - 1, cursorY + count);
                break;
            }
            case 'C': {
                int count = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
                cursorX = std::min(TERMINAL_COLS - 1, cursorX + count);
                break;
            }
            case 'D': {
                int count = (!csiParams.empty() && csiParams[0] > 0) ? csiParams[0] : 1;
                cursorX = std::max(0, cursorX - count);
                break;
            }
            case 'J': {
                int mode = !csiParams.empty() ? csiParams[0] : 0;
                ClearDisplay(mode);
                break;
            }
            case 'K': {
                int mode = !csiParams.empty() ? csiParams[0] : 0;
                ClearLine(mode);
                break;
            }
            case 'm': {
                HandleSGR();
                break;
            }
            case 'h': {
                if (isPrivateMode) {
                    for (int param : csiParams) {
                        if (param == 25) {
                            cursorVisible = true;
                        } else if (param == 1049) {
                            if (!useAltScreen) {
                                altSavedCursorX = cursorX;
                                altSavedCursorY = cursorY;
                            }
                            useAltScreen = true;
                            for (auto& row : altScreen) {
                                row.assign(TERMINAL_COLS, Cell{' ', DEFAULT_FG, DEFAULT_BG, 0});
                            }
                            cursorX = 0;
                            cursorY = 0;
                        } else if (param == 47) {
                            useAltScreen = true;
                        }
                    }
                }
                break;
            }
            case 'l': {
                if (isPrivateMode) {
                    for (int param : csiParams) {
                        if (param == 25) {
                            cursorVisible = false;
                        } else if (param == 1049) {
                            if (useAltScreen) {
                                useAltScreen = false;
                                cursorX = altSavedCursorX;
                                cursorY = altSavedCursorY;
                            }
                        } else if (param == 47) {
                            useAltScreen = false;
                        }
                    }
                }
                break;
            }
            case 'n': {
                if (!csiParams.empty() && csiParams[0] == 6 && responseCallback) {
                    std::string resp = "\x1b[" + std::to_string(cursorY + 1) + ";" + std::to_string(cursorX + 1) + "R";
                    for (char ch : resp) {
                        responseCallback(ch);
                    }
                }
                break;
            }
            case 's': {
                savedCursorX = cursorX;
                savedCursorY = cursorY;
                break;
            }
            case 'u': {
                cursorX = savedCursorX;
                cursorY = savedCursorY;
                break;
            }
            default:
                break;
        }
    }
};

static ConsoleState& GetState() {
    static ConsoleState instance;
    return instance;
}

std::mutex& GetMutex() {
    static std::mutex s_consoleMutex;
    return s_consoleMutex;
}

const Cell& GetCell(int col, int row) {
    static const Cell emptyCell{' ', DEFAULT_FG, DEFAULT_BG, 0};
    auto& state = GetState();
    const auto& grid = state.ActiveScreen();
    if (row < 0 || row >= TERMINAL_ROWS || col < 0 || col >= TERMINAL_COLS) {
        return emptyCell;
    }
    return grid[row][col];
}

int GetCursorX() { return GetState().cursorX; }
int GetCursorY() { return GetState().cursorY; }
bool IsCursorVisible() { return GetState().cursorVisible; }
size_t GetScrollbackCount() { return GetState().scrollbackLines.size(); }
const std::string& GetScrollbackLine(size_t index) {
    static const std::string emptyLine;
    auto& lines = GetState().scrollbackLines;
    if (index >= lines.size()) {
        return emptyLine;
    }
    return lines[index];
}
bool IsAlternateScreen() { return GetState().useAltScreen; }
uint32_t GetDefaultFg() { return GetState().defaultFg; }
uint32_t GetDefaultBg() { return GetState().defaultBg; }

void SetInputResponseCallback(std::function<void(char)> callback) {
    std::lock_guard<std::mutex> lock(GetMutex());
    GetState().responseCallback = std::move(callback);
}

static const std::vector<std::string>& GetLinesUnlocked() {
    auto& state = GetState();
    state.cachedLines = state.scrollbackLines;
    const auto& grid = state.ActiveScreen();
    for (int row = 0; row < TERMINAL_ROWS; ++row) {
        std::string line;
        line.reserve(TERMINAL_COLS);
        for (int col = 0; col < TERMINAL_COLS; ++col) {
            AppendUtf8(line, grid[row][col].codepoint);
        }
        while (!line.empty() && line.back() == ' ') {
            line.pop_back();
        }
        if (!line.empty() || row < state.cursorY) {
            state.cachedLines.push_back(line);
        }
    }
    return state.cachedLines;
}

static const std::string& GetCurrentLineUnlocked() {
    auto& state = GetState();
    state.cachedCurrentLine.clear();
    const auto& grid = state.ActiveScreen();
    if (state.cursorY >= 0 && state.cursorY < TERMINAL_ROWS) {
        for (int col = 0; col <= state.cursorX && col < TERMINAL_COLS; ++col) {
            AppendUtf8(state.cachedCurrentLine, grid[state.cursorY][col].codepoint);
        }
    }
    return state.cachedCurrentLine;
}

const std::vector<std::string>& GetLines() {
    std::lock_guard<std::mutex> lock(GetMutex());
    return GetLinesUnlocked();
}

const std::string& GetCurrentLine() {
    std::lock_guard<std::mutex> lock(GetMutex());
    return GetCurrentLineUnlocked();
}

void Clear() {
    std::lock_guard<std::mutex> lock(GetMutex());
    GetState().ResetBuffer();
}

static void ProcessCharUnlocked(char character) {
    auto& state = GetState();
    auto byteVal = static_cast<uint8_t>(character);

    if (character == 0x1B) {
        state.utf8Remaining = 0;
        state.parserState = ParserState::Escape;
        return;
    }
    if (character == 0x18 || character == 0x1A) {
        state.utf8Remaining = 0;
        state.parserState = ParserState::Ground;
        return;
    }

    if (state.parserState == ParserState::Ground) {
        if (state.utf8Remaining > 0) {
            if ((byteVal & 0xC0) == 0x80) {
                state.utf8Codepoint = (state.utf8Codepoint << 6) | (byteVal & 0x3F);
                state.utf8Remaining--;
                if (state.utf8Remaining == 0) {
                    state.PutChar(state.utf8Codepoint);
                }
            } else {
                state.utf8Remaining = 0;
            }
            return;
        }

        if ((byteVal & 0x80) != 0) {
            if ((byteVal & 0xE0) == 0xC0) {
                state.utf8Codepoint = byteVal & 0x1F;
                state.utf8Remaining = 1;
            } else if ((byteVal & 0xF0) == 0xE0) {
                state.utf8Codepoint = byteVal & 0x0F;
                state.utf8Remaining = 2;
            } else if ((byteVal & 0xF8) == 0xF0) {
                state.utf8Codepoint = byteVal & 0x07;
                state.utf8Remaining = 3;
            }
            return;
        }
        if (character == '\r') {
            state.cursorX = 0;
            return;
        }
        if (character == '\n') {
            state.cursorY++;
            if (state.cursorY >= TERMINAL_ROWS) {
                state.ScrollUp();
                state.cursorY = TERMINAL_ROWS - 1;
            }
            return;
        }
        if (character == 0x08 || character == 0x7F) {
            if (state.cursorX > 0) {
                state.cursorX--;
            }
            return;
        }
        if (character == '\t') {
            state.cursorX = (state.cursorX + 8) & ~7;
            if (state.cursorX >= TERMINAL_COLS) {
                state.cursorX = TERMINAL_COLS - 1;
            }
            return;
        }
        if (character >= 32) {
            uint32_t cp = static_cast<uint32_t>(byteVal);
            if (state.decSpecialGraphics) {
                cp = MapDecGraphics(character);
            }
            state.PutChar(cp);
        }
    } else if (state.parserState == ParserState::Escape) {
        if (character == '[') {
            state.parserState = ParserState::Csi;
            state.csiParams.clear();
            state.currentParam = 0;
            state.hasCurrentParam = false;
            state.isPrivateMode = false;
        } else if (character == ']') {
            state.parserState = ParserState::Osc;
            state.oscBuffer.clear();
        } else if (character == '(') {
            state.parserState = ParserState::CharsetG0;
        } else if (character == ')') {
            state.parserState = ParserState::CharsetG1;
        } else if (character == '7') {
            state.savedCursorX = state.cursorX;
            state.savedCursorY = state.cursorY;
            state.savedFg = state.currentFg;
            state.savedBg = state.currentBg;
            state.savedAttrs = state.currentAttrs;
            state.parserState = ParserState::Ground;
        } else if (character == '8') {
            state.cursorX = state.savedCursorX;
            state.cursorY = state.savedCursorY;
            state.currentFg = state.savedFg;
            state.currentBg = state.savedBg;
            state.currentAttrs = state.savedAttrs;
            state.parserState = ParserState::Ground;
        } else if (character == 'c') {
            state.ResetBuffer();
            state.parserState = ParserState::Ground;
        } else {
            state.parserState = ParserState::Ground;
        }
    } else if (state.parserState == ParserState::Osc) {
        if (character == 0x07) {
            state.ExecuteOsc();
            state.parserState = ParserState::Ground;
        } else if (character == 0x1B) {
            state.ExecuteOsc();
            state.parserState = ParserState::Escape;
        } else if (state.oscBuffer.size() < 128) {
            state.oscBuffer.push_back(character);
        } else {
            state.parserState = ParserState::Ground;
        }
    } else if (state.parserState == ParserState::CharsetG0) {
        state.decSpecialGraphics = (character == '0');
        state.parserState = ParserState::Ground;
    } else if (state.parserState == ParserState::CharsetG1) {
        state.parserState = ParserState::Ground;
    } else if (state.parserState == ParserState::Csi) {
        if (character >= '0' && character <= '9') {
            if (state.currentParam < 10000) {
                state.currentParam = state.currentParam * 10 + (character - '0');
            }
            state.hasCurrentParam = true;
        } else if (character == ';') {
            state.csiParams.push_back(state.currentParam);
            state.currentParam = 0;
            state.hasCurrentParam = false;
        } else if (character == '?') {
            state.isPrivateMode = true;
        } else {
            state.ExecuteCsi(character);
            state.parserState = ParserState::Ground;
        }
    }
}

void OutputCallback(char character) {
    std::lock_guard<std::mutex> lock(GetMutex());
    ProcessCharUnlocked(character);
}

bool SaveState(std::ostream& out) {
    std::lock_guard<std::mutex> lock(GetMutex());
    auto& state = GetState();

    Hardware::ISerializable::Serialize(out, state.useAltScreen);
    Hardware::ISerializable::Serialize(out, state.cursorX);
    Hardware::ISerializable::Serialize(out, state.cursorY);
    Hardware::ISerializable::Serialize(out, state.cursorVisible);
    Hardware::ISerializable::Serialize(out, state.currentFg);
    Hardware::ISerializable::Serialize(out, state.currentBg);
    Hardware::ISerializable::Serialize(out, state.currentAttrs);
    Hardware::ISerializable::Serialize(out, state.defaultFg);
    Hardware::ISerializable::Serialize(out, state.defaultBg);

    auto scrollbackCount = static_cast<uint32_t>(state.scrollbackLines.size());
    Hardware::ISerializable::Serialize(out, scrollbackCount);
    for (const auto& line : state.scrollbackLines) {
        Hardware::ISerializable::Serialize(out, line);
    }

    auto serializeGrid = [&out](const std::vector<std::vector<Cell>>& grid) {
        for (int r = 0; r < TERMINAL_ROWS; ++r) {
            for (int c = 0; c < TERMINAL_COLS; ++c) {
                static const Cell defaultCell{' ', DEFAULT_FG, DEFAULT_BG, 0};
                const auto& cell = (r < static_cast<int>(grid.size()) && c < static_cast<int>(grid[r].size()))
                                       ? grid[r][c]
                                       : defaultCell;
                Hardware::ISerializable::Serialize(out, cell.codepoint);
                Hardware::ISerializable::Serialize(out, cell.fg);
                Hardware::ISerializable::Serialize(out, cell.bg);
                Hardware::ISerializable::Serialize(out, cell.attributes);
            }
        }
    };

    serializeGrid(state.screen);
    serializeGrid(state.altScreen);

    return out.good();
}

bool LoadState(std::istream& inputStream) {
    std::lock_guard<std::mutex> lock(GetMutex());
    auto& state = GetState();
    state.ResetBuffer();

    Hardware::ISerializable::Deserialize(inputStream, state.useAltScreen);
    Hardware::ISerializable::Deserialize(inputStream, state.cursorX);
    Hardware::ISerializable::Deserialize(inputStream, state.cursorY);
    Hardware::ISerializable::Deserialize(inputStream, state.cursorVisible);
    Hardware::ISerializable::Deserialize(inputStream, state.currentFg);
    Hardware::ISerializable::Deserialize(inputStream, state.currentBg);
    Hardware::ISerializable::Deserialize(inputStream, state.currentAttrs);
    Hardware::ISerializable::Deserialize(inputStream, state.defaultFg);
    Hardware::ISerializable::Deserialize(inputStream, state.defaultBg);
    if (!inputStream.good()) {
        return false;
    }

    state.cursorX = std::clamp(state.cursorX, 0, TERMINAL_COLS);
    state.cursorY = std::clamp(state.cursorY, 0, TERMINAL_ROWS - 1);

    uint32_t scrollbackCount = 0;
    Hardware::ISerializable::Deserialize(inputStream, scrollbackCount);
    if (!inputStream.good() || scrollbackCount > CONSOLE_MAX_LINES) {
        return false;
    }
    state.scrollbackLines.resize(scrollbackCount);
    for (uint32_t i = 0; i < scrollbackCount; ++i) {
        Hardware::ISerializable::Deserialize(inputStream, state.scrollbackLines[i]);
        if (!inputStream.good()) {
            return false;
        }
    }

    auto deserializeGrid = [&inputStream](std::vector<std::vector<Cell>>& grid) -> bool {
        for (int r = 0; r < TERMINAL_ROWS; ++r) {
            for (int c = 0; c < TERMINAL_COLS; ++c) {
                Cell& cell = grid[r][c];
                Hardware::ISerializable::Deserialize(inputStream, cell.codepoint);
                Hardware::ISerializable::Deserialize(inputStream, cell.fg);
                Hardware::ISerializable::Deserialize(inputStream, cell.bg);
                Hardware::ISerializable::Deserialize(inputStream, cell.attributes);
                if (!inputStream.good()) {
                    return false;
                }
            }
        }
        return true;
    };

    if (!deserializeGrid(state.screen) || !deserializeGrid(state.altScreen)) {
        return false;
    }

    state.cachedLines.clear();
    state.cachedCurrentLine.clear();

    return inputStream.good();
}

}  // namespace Console
