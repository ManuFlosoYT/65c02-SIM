#include "Frontend/Control/Console.h"
#include "Hardware/Core/ISerializable.h"
#include <cstdint>

namespace Console {

struct ConsoleState {
    std::vector<std::string> consoleLines;
    std::string currentLine;
    int cursorX = 0;
};

static ConsoleState& GetState() {
    static ConsoleState instance;
    return instance;
}

const std::vector<std::string>& GetLines() { return GetState().consoleLines; }
const std::string& GetCurrentLine() { return GetState().currentLine; }

void Clear() {
    auto& state = GetState();
    state.consoleLines.clear();
    state.currentLine.clear();
    state.cursorX = 0;
}

void OutputCallback(char character) {
    auto& state = GetState();
    if (character == '\r') {
        state.cursorX = 0;
    } else if (character == '\n') {
        state.consoleLines.push_back(state.currentLine);
        if (state.consoleLines.size() > CONSOLE_MAX_LINES) {
            state.consoleLines.erase(state.consoleLines.begin());
        }
        state.currentLine.clear();
        state.cursorX = 0;
    } else if (character == 0x08 || character == 0x7F) {  // Backspace
        if (state.cursorX > 0) {
            state.cursorX--;
        }
    } else if (character >= 32) {
        if (state.cursorX >= static_cast<int>(state.currentLine.size())) {
            state.currentLine.resize(state.cursorX + 1, ' ');
        }
        state.currentLine[state.cursorX] = character;
        state.cursorX++;
    }
}
void Print(const std::string& message) {
    for (char character : message) {
        OutputCallback(character);
    }
    OutputCallback('\n');
}

bool SaveState(std::ostream& out) {
    auto& state = GetState();
    auto linesCount = static_cast<uint32_t>(state.consoleLines.size());
    Hardware::ISerializable::Serialize(out, linesCount);
    for (const auto& line : state.consoleLines) {
        auto len = static_cast<uint32_t>(line.size());
        Hardware::ISerializable::Serialize(out, len);
        out.write(line.c_str(), static_cast<std::streamsize>(len));
    }

    auto currentLen = static_cast<uint32_t>(state.currentLine.size());
    Hardware::ISerializable::Serialize(out, currentLen);
    out.write(state.currentLine.c_str(), static_cast<std::streamsize>(currentLen));

    Hardware::ISerializable::Serialize(out, state.cursorX);

    return out.good();
}

bool LoadState(std::istream& inputStream) {
    auto& state = GetState();
    uint32_t linesCount = 0;
    Hardware::ISerializable::Deserialize(inputStream, linesCount);
    state.consoleLines.clear();
    for (uint32_t i = 0; i < linesCount; ++i) {
        uint32_t len = 0;
        Hardware::ISerializable::Deserialize(inputStream, len);
        std::string line(len, '\0');
        inputStream.read(line.data(), static_cast<std::streamsize>(len));
        state.consoleLines.push_back(line);
    }

    uint32_t currentLen = 0;
    Hardware::ISerializable::Deserialize(inputStream, currentLen);
    state.currentLine.resize(currentLen);
    inputStream.read(state.currentLine.data(), static_cast<std::streamsize>(currentLen));

    Hardware::ISerializable::Deserialize(inputStream, state.cursorX);

    return inputStream.good();
}

}  // namespace Console
