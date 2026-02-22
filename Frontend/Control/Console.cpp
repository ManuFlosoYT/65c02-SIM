#include "Frontend/Control/Console.h"

namespace Console {

std::vector<std::string> consoleLines;
std::string currentLine;
int cursorX = 0;

void Clear() {
    consoleLines.clear();
    currentLine.clear();
    cursorX = 0;
}

void OutputCallback(char c) {
    if (c == '\r') {
        cursorX = 0;
    } else if (c == '\n') {
        consoleLines.push_back(currentLine);
        if (consoleLines.size() > CONSOLE_MAX_LINES) {
            consoleLines.erase(consoleLines.begin());
        }
        currentLine.clear();
        cursorX = 0;
    } else if (c == 0x08 || c == 0x7F) {  // Backspace
        if (cursorX > 0) cursorX--;
    } else if (c >= 32) {
        if (cursorX >= (int)currentLine.size()) {
            currentLine.resize(cursorX + 1, ' ');
        }
        currentLine[cursorX] = c;
        cursorX++;
    }
}

bool SaveState(std::ostream& out) {
    size_t linesCount = consoleLines.size();
    out.write(reinterpret_cast<const char*>(&linesCount), sizeof(linesCount));
    for (const auto& line : consoleLines) {
        size_t len = line.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(line.c_str(), len);
    }

    size_t currentLen = currentLine.size();
    out.write(reinterpret_cast<const char*>(&currentLen), sizeof(currentLen));
    out.write(currentLine.c_str(), currentLen);

    out.write(reinterpret_cast<const char*>(&cursorX), sizeof(cursorX));

    return out.good();
}

bool LoadState(std::istream& in) {
    size_t linesCount = 0;
    in.read(reinterpret_cast<char*>(&linesCount), sizeof(linesCount));
    consoleLines.clear();
    for (size_t i = 0; i < linesCount; ++i) {
        size_t len = 0;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string line(len, '\0');
        in.read(&line[0], len);
        consoleLines.push_back(line);
    }

    size_t currentLen = 0;
    in.read(reinterpret_cast<char*>(&currentLen), sizeof(currentLen));
    currentLine.resize(currentLen);
    in.read(&currentLine[0], currentLen);

    in.read(reinterpret_cast<char*>(&cursorX), sizeof(cursorX));

    return in.good();
}

}  // namespace Console
