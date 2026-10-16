#include "Frontend/Control/Console.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>

#include "Hardware/Core/ISerializable.h"

namespace Console {

static constexpr std::array<uint32_t, 16> ANSI_PALETTE = {
    0xFF000000, 0xFF2B31CD, 0xFF79BC0D, 0xFF10E5E5, 0xFFC87224, 0xFFBC3FBC, 0xFFCDA811, 0xFFE5E5E5,
    0xFF666666, 0xFF4C4CF1, 0xFF8BD123, 0xFF43F5F5, 0xFFEA8E3B, 0xFFD670D6, 0xFFDBB829, 0xFFFFFFFF};

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

enum class ParserState { Ground, Escape, Csi, CharsetG0, CharsetG1 };

struct ConsoleState {
    std::vector<std::vector<Cell>> screen;
    std::vector<std::vector<Cell>> altScreen;
    bool useAltScreen = false;
    std::vector<std::string> scrollbackLines;

    int cursorX = 0;
    int cursorY = 0;
    int savedCursorX = 0;
    int savedCursorY = 0;
    bool cursorVisible = true;

    uint32_t currentFg = DEFAULT_FG;
    uint32_t currentBg = DEFAULT_BG;
    uint8_t currentAttrs = 0;
    uint32_t savedFg = DEFAULT_FG;
    uint32_t savedBg = DEFAULT_BG;
    uint8_t savedAttrs = 0;

    bool decSpecialGraphics = false;
    std::function<void(char)> responseCallback;

    ParserState parserState = ParserState::Ground;
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
        screen.assign(TERMINAL_ROWS, std::vector<Cell>(TERMINAL_COLS));
        altScreen.assign(TERMINAL_ROWS, std::vector<Cell>(TERMINAL_COLS));
        useAltScreen = false;
        scrollbackLines.clear();
        cursorX = 0;
        cursorY = 0;
        savedCursorX = 0;
        savedCursorY = 0;
        cursorVisible = true;
        currentFg = DEFAULT_FG;
        currentBg = DEFAULT_BG;
        currentAttrs = 0;
        decSpecialGraphics = false;
        parserState = ParserState::Ground;
        utf8Remaining = 0;
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
                uint32_t cp = grid[0][col].codepoint;
                if (cp < 128) {
                    line.push_back(static_cast<char>(cp));
                } else {
                    line.push_back('?');
                }
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
        for (int param : csiParams) {
            if (param == 0) {
                currentFg = DEFAULT_FG;
                currentBg = DEFAULT_BG;
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
            } else if (param == 39) {
                currentFg = DEFAULT_FG;
            } else if (param >= 40 && param <= 47) {
                currentBg = ANSI_PALETTE[param - 40];
            } else if (param == 49) {
                currentBg = DEFAULT_BG;
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
                        } else if (param == 47 || param == 1049) {
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
                        } else if (param == 47 || param == 1049) {
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

std::recursive_mutex& GetMutex() {
    static std::recursive_mutex s_consoleMutex;
    return s_consoleMutex;
}

const Cell& GetCell(int col, int row) {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    static const Cell emptyCell{' ', DEFAULT_FG, DEFAULT_BG, 0};
    auto& state = GetState();
    const auto& grid = state.ActiveScreen();
    if (row < 0 || row >= TERMINAL_ROWS || col < 0 || col >= TERMINAL_COLS) {
        return emptyCell;
    }
    return grid[row][col];
}

int GetCursorX() {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    return GetState().cursorX;
}
int GetCursorY() {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    return GetState().cursorY;
}
bool IsCursorVisible() {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    return GetState().cursorVisible;
}
size_t GetScrollbackCount() {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    return GetState().scrollbackLines.size();
}
const std::string& GetScrollbackLine(size_t index) {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    static const std::string emptyLine;
    auto& lines = GetState().scrollbackLines;
    if (index >= lines.size()) {
        return emptyLine;
    }
    return lines[index];
}
bool IsAlternateScreen() {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    return GetState().useAltScreen;
}

void SetInputResponseCallback(std::function<void(char)> callback) {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    GetState().responseCallback = std::move(callback);
}

const std::vector<std::string>& GetLines() {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    auto& state = GetState();
    state.cachedLines = state.scrollbackLines;
    const auto& grid = state.ActiveScreen();
    for (int row = 0; row < TERMINAL_ROWS; ++row) {
        std::string line;
        line.reserve(TERMINAL_COLS);
        for (int col = 0; col < TERMINAL_COLS; ++col) {
            uint32_t cp = grid[row][col].codepoint;
            if (cp < 128) {
                line.push_back(static_cast<char>(cp));
            } else {
                line.push_back('?');
            }
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

const std::string& GetCurrentLine() {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    auto& state = GetState();
    state.cachedCurrentLine.clear();
    const auto& grid = state.ActiveScreen();
    if (state.cursorY >= 0 && state.cursorY < TERMINAL_ROWS) {
        for (int col = 0; col <= state.cursorX && col < TERMINAL_COLS; ++col) {
            uint32_t cp = grid[state.cursorY][col].codepoint;
            if (cp < 128) {
                state.cachedCurrentLine.push_back(static_cast<char>(cp));
            }
        }
    }
    return state.cachedCurrentLine;
}

void Clear() {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    GetState().ResetBuffer();
}

void OutputCallback(char character) {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    auto& state = GetState();
    auto byteVal = static_cast<uint8_t>(character);

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

        if (character == 0x1B) {
            state.parserState = ParserState::Escape;
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
    } else if (state.parserState == ParserState::CharsetG0) {
        state.decSpecialGraphics = (character == '0');
        state.parserState = ParserState::Ground;
    } else if (state.parserState == ParserState::CharsetG1) {
        state.parserState = ParserState::Ground;
    } else if (state.parserState == ParserState::Csi) {
        if (character >= '0' && character <= '9') {
            state.currentParam = state.currentParam * 10 + (character - '0');
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

void Print(const std::string& message) {
    for (char character : message) {
        OutputCallback(character);
    }
    OutputCallback('\n');
}

bool SaveState(std::ostream& out) {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    auto& state = GetState();
    auto lines = GetLines();
    auto linesCount = static_cast<uint32_t>(lines.size());
    Hardware::ISerializable::Serialize(out, linesCount);
    for (const auto& line : lines) {
        auto len = static_cast<uint32_t>(line.size());
        Hardware::ISerializable::Serialize(out, len);
        out.write(line.c_str(), static_cast<std::streamsize>(len));
    }

    const auto& currentLine = GetCurrentLine();
    auto currentLen = static_cast<uint32_t>(currentLine.size());
    Hardware::ISerializable::Serialize(out, currentLen);
    out.write(currentLine.c_str(), static_cast<std::streamsize>(currentLen));

    Hardware::ISerializable::Serialize(out, state.cursorX);
    Hardware::ISerializable::Serialize(out, state.cursorY);

    return out.good();
}

bool LoadState(std::istream& inputStream) {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    auto& state = GetState();
    state.ResetBuffer();

    uint32_t linesCount = 0;
    Hardware::ISerializable::Deserialize(inputStream, linesCount);
    for (uint32_t i = 0; i < linesCount; ++i) {
        uint32_t len = 0;
        Hardware::ISerializable::Deserialize(inputStream, len);
        std::string line(len, '\0');
        inputStream.read(line.data(), static_cast<std::streamsize>(len));
        state.scrollbackLines.push_back(line);
    }

    uint32_t currentLen = 0;
    Hardware::ISerializable::Deserialize(inputStream, currentLen);
    std::string currentLine(currentLen, '\0');
    inputStream.read(currentLine.data(), static_cast<std::streamsize>(currentLen));

    Hardware::ISerializable::Deserialize(inputStream, state.cursorX);
    if (inputStream.good()) {
        Hardware::ISerializable::Deserialize(inputStream, state.cursorY);
    }

    return inputStream.good();
}

}  // namespace Console
