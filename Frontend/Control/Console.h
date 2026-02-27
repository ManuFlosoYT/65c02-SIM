#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace Console {

[[nodiscard]] const std::vector<std::string>& GetLines();
[[nodiscard]] const std::string& GetCurrentLine();

constexpr size_t CONSOLE_MAX_LINES = 1000;

void Clear();
void OutputCallback(char character);

bool SaveState(std::ostream& out);
bool LoadState(std::istream& inputStream);

}  // namespace Console
