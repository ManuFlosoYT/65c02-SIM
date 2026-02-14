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

}  // namespace Console
