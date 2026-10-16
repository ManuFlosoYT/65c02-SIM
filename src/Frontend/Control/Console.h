#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace Console {

constexpr int TERMINAL_COLS = 80;
constexpr int TERMINAL_ROWS = 25;
constexpr size_t CONSOLE_MAX_LINES = 1000;

constexpr uint8_t ATTR_BOLD = 1 << 0;
constexpr uint8_t ATTR_UNDERLINE = 1 << 1;
constexpr uint8_t ATTR_INVERSE = 1 << 2;

constexpr uint32_t DEFAULT_FG = 0xFFCCCCCC;
constexpr uint32_t DEFAULT_BG = 0x00000000;

struct Cell {
    uint32_t codepoint = ' ';
    uint32_t fg = DEFAULT_FG;
    uint32_t bg = DEFAULT_BG;
    uint8_t attributes = 0;
};

[[nodiscard]] const std::vector<std::string>& GetLines();
[[nodiscard]] const std::string& GetCurrentLine();

[[nodiscard]] const Cell& GetCell(int col, int row);
[[nodiscard]] int GetCursorX();
[[nodiscard]] int GetCursorY();
[[nodiscard]] bool IsCursorVisible();
[[nodiscard]] size_t GetScrollbackCount();
[[nodiscard]] const std::string& GetScrollbackLine(size_t index);
[[nodiscard]] bool IsAlternateScreen();
[[nodiscard]] std::recursive_mutex& GetMutex();

void Clear();
void OutputCallback(char character);
void Print(const std::string& message);
void SetInputResponseCallback(std::function<void(char)> callback);

bool SaveState(std::ostream& out);
bool LoadState(std::istream& inputStream);

}  // namespace Console
