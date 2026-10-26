#include "Hardware/Core/TerminalBuffer.h"

#include <algorithm>

#include "Hardware/Core/ISerializable.h"

namespace Console {

void AppendUtf8(std::string& out, uint32_t cp) {
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

TerminalBuffer::TerminalBuffer() {
    ResetBuffer();
}

void TerminalBuffer::ResetBuffer() {
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
    cachedLines.clear();
    cachedCurrentLine.clear();
}

std::vector<std::vector<Cell>>& TerminalBuffer::ActiveScreen() {
    return useAltScreen ? altScreen : screen;
}

const std::vector<std::vector<Cell>>& TerminalBuffer::ActiveScreen() const {
    return useAltScreen ? altScreen : screen;
}

void TerminalBuffer::ScrollUp() {
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

void TerminalBuffer::PutChar(uint32_t codepoint) {
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

void TerminalBuffer::ClearDisplay(int mode) {
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

void TerminalBuffer::ClearLine(int mode) {
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

const std::vector<std::string>& TerminalBuffer::GetLinesUnlocked() {
    cachedLines = scrollbackLines;
    const auto& grid = ActiveScreen();
    for (int row = 0; row < TERMINAL_ROWS; ++row) {
        std::string line;
        line.reserve(TERMINAL_COLS);
        for (int col = 0; col < TERMINAL_COLS; ++col) {
            AppendUtf8(line, grid[row][col].codepoint);
        }
        while (!line.empty() && line.back() == ' ') {
            line.pop_back();
        }
        if (!line.empty() || row < cursorY) {
            cachedLines.push_back(line);
        }
    }
    return cachedLines;
}

const std::string& TerminalBuffer::GetCurrentLineUnlocked() {
    cachedCurrentLine.clear();
    const auto& grid = ActiveScreen();
    if (cursorY >= 0 && cursorY < TERMINAL_ROWS) {
        for (int col = 0; col <= cursorX && col < TERMINAL_COLS; ++col) {
            AppendUtf8(cachedCurrentLine, grid[cursorY][col].codepoint);
        }
    }
    return cachedCurrentLine;
}

bool TerminalBuffer::SaveState(std::ostream& out) const {
    Hardware::ISerializable::Serialize(out, useAltScreen);
    Hardware::ISerializable::Serialize(out, cursorX);
    Hardware::ISerializable::Serialize(out, cursorY);
    Hardware::ISerializable::Serialize(out, cursorVisible);
    Hardware::ISerializable::Serialize(out, currentFg);
    Hardware::ISerializable::Serialize(out, currentBg);
    Hardware::ISerializable::Serialize(out, currentAttrs);
    Hardware::ISerializable::Serialize(out, defaultFg);
    Hardware::ISerializable::Serialize(out, defaultBg);

    auto scrollbackCount = static_cast<uint32_t>(scrollbackLines.size());
    Hardware::ISerializable::Serialize(out, scrollbackCount);
    for (const auto& line : scrollbackLines) {
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

    serializeGrid(screen);
    serializeGrid(altScreen);

    return out.good();
}

bool TerminalBuffer::LoadState(std::istream& inputStream) {
    ResetBuffer();

    Hardware::ISerializable::Deserialize(inputStream, useAltScreen);
    Hardware::ISerializable::Deserialize(inputStream, cursorX);
    Hardware::ISerializable::Deserialize(inputStream, cursorY);
    Hardware::ISerializable::Deserialize(inputStream, cursorVisible);
    Hardware::ISerializable::Deserialize(inputStream, currentFg);
    Hardware::ISerializable::Deserialize(inputStream, currentBg);
    Hardware::ISerializable::Deserialize(inputStream, currentAttrs);
    Hardware::ISerializable::Deserialize(inputStream, defaultFg);
    Hardware::ISerializable::Deserialize(inputStream, defaultBg);
    if (!inputStream.good()) {
        return false;
    }

    cursorX = std::clamp(cursorX, 0, TERMINAL_COLS);
    cursorY = std::clamp(cursorY, 0, TERMINAL_ROWS - 1);

    uint32_t scrollbackCount = 0;
    Hardware::ISerializable::Deserialize(inputStream, scrollbackCount);
    if (!inputStream.good() || scrollbackCount > CONSOLE_MAX_LINES) {
        return false;
    }
    scrollbackLines.resize(scrollbackCount);
    for (uint32_t i = 0; i < scrollbackCount; ++i) {
        Hardware::ISerializable::Deserialize(inputStream, scrollbackLines[i]);
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

    if (!deserializeGrid(screen) || !deserializeGrid(altScreen)) {
        return false;
    }

    cachedLines.clear();
    cachedCurrentLine.clear();

    return inputStream.good();
}

}  // namespace Console
