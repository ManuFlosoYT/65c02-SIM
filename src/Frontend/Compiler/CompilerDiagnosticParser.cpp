#ifndef TARGET_WASM
#include "CompilerDiagnosticParser.h"
#include <algorithm>
#include <cctype>
#include <sstream>

bool CompilerDiagnosticParser::IsSizeOverflowError(const std::string& log) {
    return log.find("will not fit in region") != std::string::npos ||
           log.find("overflowed by") != std::string::npos ||
           log.find("address range overlaps with") != std::string::npos;
}

std::vector<CompilerFrontend::Diagnostic> CompilerDiagnosticParser::ParseDiagnostics(const std::string& log) {
    std::vector<CompilerFrontend::Diagnostic> diagnostics;
    std::istringstream stream(log);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        size_t markerPos = line.find(": error: ");
        size_t markerLen = 9;
        bool isError = true;

        if (markerPos == std::string::npos) {
            markerPos = line.find(": fatal error: ");
            markerLen = 15;
            isError = true;
        }
        if (markerPos == std::string::npos) {
            markerPos = line.find(": warning: ");
            markerLen = 11;
            isError = false;
        }
        if (markerPos == std::string::npos) {
            continue;
        }

        std::string message = line.substr(markerPos + markerLen);
        std::string prefix = line.substr(0, markerPos);

        size_t secondColon = prefix.rfind(':');
        if (secondColon == std::string::npos) {
            diagnostics.push_back({.file = prefix, .line = 0, .column = 0, .message = message, .isError = isError});
            continue;
        }

        std::string secondPart = prefix.substr(secondColon + 1);
        bool secondIsDigit = !secondPart.empty() && std::ranges::all_of(secondPart, [](char c) {
            return std::isdigit(static_cast<unsigned char>(c)) != 0;
        });

        if (!secondIsDigit) {
            diagnostics.push_back({.file = prefix, .line = 0, .column = 0, .message = message, .isError = isError});
            continue;
        }

        size_t firstColon = prefix.rfind(':', secondColon - 1);
        if (firstColon != std::string::npos && firstColon > 1) {
            std::string firstPart = prefix.substr(firstColon + 1, secondColon - firstColon - 1);
            bool firstIsDigit = !firstPart.empty() && std::ranges::all_of(firstPart, [](char c) {
                return std::isdigit(static_cast<unsigned char>(c)) != 0;
            });

            if (firstIsDigit) {
                try {
                    int lineNum = std::stoi(firstPart);
                    int colNum = std::stoi(secondPart);
                    std::string file = prefix.substr(0, firstColon);
                    diagnostics.push_back(
                        {.file = file, .line = lineNum, .column = colNum, .message = message, .isError = isError});
                    continue;
                } catch (const std::exception&) {
                }
            }
        }

        try {
            int lineNum = std::stoi(secondPart);
            std::string file = prefix.substr(0, secondColon);
            diagnostics.push_back(
                {.file = file, .line = lineNum, .column = 0, .message = message, .isError = isError});
        } catch (const std::exception&) {
            diagnostics.push_back({.file = prefix, .line = 0, .column = 0, .message = message, .isError = isError});
        }
    }

    return diagnostics;
}

#endif
