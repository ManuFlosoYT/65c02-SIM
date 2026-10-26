#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "Hardware/Core/Console.h"

namespace Console {

struct TerminalBuffer {
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

    std::vector<std::string> cachedLines;
    std::string cachedCurrentLine;

    TerminalBuffer();

    void ResetBuffer();
    [[nodiscard]] std::vector<std::vector<Cell>>& ActiveScreen();
    [[nodiscard]] const std::vector<std::vector<Cell>>& ActiveScreen() const;

    void ScrollUp();
    void PutChar(uint32_t codepoint);
    void ClearDisplay(int mode);
    void ClearLine(int mode);

    const std::vector<std::string>& GetLinesUnlocked();
    const std::string& GetCurrentLineUnlocked();

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& inputStream);
};

void AppendUtf8(std::string& out, uint32_t cp);

}  // namespace Console
