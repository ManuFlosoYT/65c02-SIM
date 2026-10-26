#include "Hardware/Core/Console.h"

#include <mutex>

#include "Hardware/Core/AnsiVt100Parser.h"
#include "Hardware/Core/TerminalBuffer.h"

namespace Console {

struct ConsoleContext {
    TerminalBuffer buffer;
    AnsiVt100Parser parser;
};

static ConsoleContext& GetContext() {
    static ConsoleContext instance;
    return instance;
}

std::mutex& GetMutex() {
    static std::mutex s_consoleMutex;
    return s_consoleMutex;
}

const Cell& GetCell(int col, int row) {
    static const Cell emptyCell{' ', DEFAULT_FG, DEFAULT_BG, 0};
    auto& ctx = GetContext();
    const auto& grid = ctx.buffer.ActiveScreen();
    if (row < 0 || row >= TERMINAL_ROWS || col < 0 || col >= TERMINAL_COLS) {
        return emptyCell;
    }
    return grid[row][col];
}

int GetCursorX() { return GetContext().buffer.cursorX; }
int GetCursorY() { return GetContext().buffer.cursorY; }
bool IsCursorVisible() { return GetContext().buffer.cursorVisible; }
size_t GetScrollbackCount() { return GetContext().buffer.scrollbackLines.size(); }

const std::string& GetScrollbackLine(size_t index) {
    static const std::string emptyLine;
    auto& lines = GetContext().buffer.scrollbackLines;
    if (index >= lines.size()) {
        return emptyLine;
    }
    return lines[index];
}

bool IsAlternateScreen() { return GetContext().buffer.useAltScreen; }
uint32_t GetDefaultFg() { return GetContext().buffer.defaultFg; }
uint32_t GetDefaultBg() { return GetContext().buffer.defaultBg; }

void SetInputResponseCallback(std::function<void(char)> callback) {
    std::lock_guard<std::mutex> lock(GetMutex());
    GetContext().parser.SetResponseCallback(std::move(callback));
}

const std::vector<std::string>& GetLines() {
    std::lock_guard<std::mutex> lock(GetMutex());
    return GetContext().buffer.GetLinesUnlocked();
}

const std::string& GetCurrentLine() {
    std::lock_guard<std::mutex> lock(GetMutex());
    return GetContext().buffer.GetCurrentLineUnlocked();
}

void Clear() {
    std::lock_guard<std::mutex> lock(GetMutex());
    GetContext().buffer.ResetBuffer();
    GetContext().parser.Reset();
}

void OutputCallback(char character) {
    std::lock_guard<std::mutex> lock(GetMutex());
    GetContext().parser.ProcessChar(character, GetContext().buffer);
}

bool SaveState(std::ostream& out) {
    std::lock_guard<std::mutex> lock(GetMutex());
    return GetContext().buffer.SaveState(out);
}

bool LoadState(std::istream& inputStream) {
    std::lock_guard<std::mutex> lock(GetMutex());
    auto& ctx = GetContext();
    ctx.parser.Reset();
    return ctx.buffer.LoadState(inputStream);
}

}  // namespace Console
