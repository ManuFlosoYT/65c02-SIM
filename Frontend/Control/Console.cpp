#include "Frontend/Control/Console.h"
#include "Hardware/Core/ISerializable.h"
#include <cstdint>

namespace Console {

namespace {
std::vector<std::string> consoleLines;
std::string currentLine;
int cursorX = 0;
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
        if (cursorX >= static_cast<int>(currentLine.size())) {
            currentLine.resize(cursorX + 1, ' ');
        }
        currentLine[cursorX] = character;
        cursorX++;
    }
}
void Print(const std::string& message) {
    for (char character : message) {
        OutputCallback(character);
    }
    OutputCallback('\n');
}

bool SaveState(std::ostream& out) {
    auto linesCount = static_cast<uint32_t>(consoleLines.size());
    Hardware::ISerializable::Serialize(out, linesCount);
    for (const auto& line : consoleLines) {
        auto len = static_cast<uint32_t>(line.size());
        Hardware::ISerializable::Serialize(out, len);
        out.write(line.c_str(), static_cast<std::streamsize>(len));
    }

    auto currentLen = static_cast<uint32_t>(currentLine.size());
    Hardware::ISerializable::Serialize(out, currentLen);
    out.write(currentLine.c_str(), static_cast<std::streamsize>(currentLen));

    Hardware::ISerializable::Serialize(out, cursorX);

    return out.good();
}

bool LoadState(std::istream& inputStream) {
    uint32_t linesCount = 0;
    Hardware::ISerializable::Deserialize(inputStream, linesCount);
    consoleLines.clear();
    for (uint32_t i = 0; i < linesCount; ++i) {
        uint32_t len = 0;
        Hardware::ISerializable::Deserialize(inputStream, len);
        std::string line(len, '\0');
        inputStream.read(line.data(), static_cast<std::streamsize>(len));
        consoleLines.push_back(line);
    }

    uint32_t currentLen = 0;
    Hardware::ISerializable::Deserialize(inputStream, currentLen);
    currentLine.resize(currentLen);
    inputStream.read(currentLine.data(), static_cast<std::streamsize>(currentLen));

    Hardware::ISerializable::Deserialize(inputStream, cursorX);

    return inputStream.good();
}

}  // namespace Console
