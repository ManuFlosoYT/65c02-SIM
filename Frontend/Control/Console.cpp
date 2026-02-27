#include "Frontend/Control/Console.h"

namespace Console {

namespace {
std::vector<std::string> consoleLines;  // NOLINT
std::string currentLine;                // NOLINT
int cursorX = 0;                        // NOLINT
}  // namespace

const std::vector<std::string>& GetLines() { return consoleLines; }
const std::string& GetCurrentLine() { return currentLine; }

void Clear() {
    consoleLines.clear();
    currentLine.clear();
    cursorX = 0;
}

void OutputCallback(char character) {
    if (character == '\r') {
        cursorX = 0;
    } else if (character == '\n') {
        consoleLines.push_back(currentLine);
        if (consoleLines.size() > CONSOLE_MAX_LINES) {
            consoleLines.erase(consoleLines.begin());
        }
        currentLine.clear();
        cursorX = 0;
    } else if (character == 0x08 || character == 0x7F) {  // Backspace
        if (cursorX > 0) {
            cursorX--;
        }
    } else if (character >= 32) {
        if (cursorX >= (int)currentLine.size()) {
            currentLine.resize(cursorX + 1, ' ');
        }
        currentLine[cursorX] = character;
        cursorX++;
    }
}

bool SaveState(std::ostream& out) {
    size_t linesCount = consoleLines.size();
    out.write(reinterpret_cast<const char*>(&linesCount), sizeof(linesCount));  // NOLINT
    for (const auto& line : consoleLines) {
        size_t len = line.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));  // NOLINT
        out.write(line.c_str(), static_cast<std::streamsize>(len));
    }

    size_t currentLen = currentLine.size();
    out.write(reinterpret_cast<const char*>(&currentLen), sizeof(currentLen));  // NOLINT
    out.write(currentLine.c_str(), static_cast<std::streamsize>(currentLen));

    out.write(reinterpret_cast<const char*>(&cursorX), sizeof(cursorX));  // NOLINT

    return out.good();
}

bool LoadState(std::istream& inputStream) {
    size_t linesCount = 0;
    inputStream.read(reinterpret_cast<char*>(&linesCount), sizeof(linesCount));  // NOLINT
    consoleLines.clear();
    for (size_t i = 0; i < linesCount; ++i) {
        size_t len = 0;
        inputStream.read(reinterpret_cast<char*>(&len), sizeof(len));  // NOLINT
        std::string line(len, '\0');
        inputStream.read(line.data(), static_cast<std::streamsize>(len));
        consoleLines.push_back(line);
    }

    size_t currentLen = 0;
    inputStream.read(reinterpret_cast<char*>(&currentLen), sizeof(currentLen));  // NOLINT
    currentLine.resize(currentLen);
    inputStream.read(currentLine.data(), static_cast<std::streamsize>(currentLen));

    inputStream.read(reinterpret_cast<char*>(&cursorX), sizeof(cursorX));  // NOLINT

    return inputStream.good();
}

}  // namespace Console
