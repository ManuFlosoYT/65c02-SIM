#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace Console {

extern std::vector<std::string> consoleLines;
extern std::string currentLine;
extern int cursorX;
constexpr size_t CONSOLE_MAX_LINES = 1000;

void Clear();
void OutputCallback(char c);

bool SaveState(std::ostream& out);
bool LoadState(std::istream& in);

}  // namespace Console
