#pragma once
#include <string>

namespace GUI {

inline void StripSpacesFromTrailingBlankLines(std::string& code) {
    size_t end = code.size();
    while (end > 0) {
        size_t lineEnd = end;
        if (code[end - 1] == '\n') {
            lineEnd = end - 1;
            if (lineEnd > 0 && code[lineEnd - 1] == '\r') {
                --lineEnd;
            }
        }

        size_t prevNewline = (lineEnd == 0) ? std::string::npos : code.rfind('\n', lineEnd - 1);
        size_t lineStart = (prevNewline == std::string::npos) ? 0 : prevNewline + 1;

        bool hasNonWhitespace = false;
        for (size_t i = lineStart; i < lineEnd; ++i) {
            char c = code[i];
            if (c != ' ' && c != '\t' && c != '\r') {
                hasNonWhitespace = true;
                break;
            }
        }

        if (hasNonWhitespace) {
            break;
        }

        if (lineEnd > lineStart) {
            code.erase(lineStart, lineEnd - lineStart);
        }

        if (prevNewline == std::string::npos) {
            break;
        }
        end = prevNewline + 1;
    }
}

inline void StripTrailingNewline(std::string& text) {
    if (text.ends_with("\r\n")) {
        text.erase(text.size() - 2);
    } else if (text.ends_with('\n')) {
        text.pop_back();
    }
}

}
