#include "Frontend/GUI/Debugger/TrafficPayloadFormatter.h"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <format>

#include "Frontend/GUI/Style/Style.h"

namespace GUI {

std::vector<std::string> TrafficPayloadFormatter::FormatHexDumpLines(std::string_view data, size_t bytesPerLine) {
    if (data.empty() || bytesPerLine == 0) {
        return {};
    }

    std::vector<std::string> lines;
    lines.reserve((data.size() + bytesPerLine - 1) / bytesPerLine);

    for (size_t i = 0; i < data.size(); i += bytesPerLine) {
        size_t count = std::min(bytesPerLine, data.size() - i);
        std::string line = std::format("{:04X}: ", i);

        for (size_t j = 0; j < bytesPerLine; ++j) {
            if (j < count) {
                line += std::format("{:02X} ", static_cast<unsigned char>(data[i + j]));
            } else {
                line += "   ";
            }
            if (j == 7) {
                line += " ";
            }
        }
        line += " |";
        for (size_t j = 0; j < count; ++j) {
            unsigned char c = static_cast<unsigned char>(data[i + j]);
            line += (std::isprint(c) != 0) ? static_cast<char>(c) : '.';
        }
        line += "|";
        lines.push_back(std::move(line));
    }

    return lines;
}

std::string TrafficPayloadFormatter::FormatHexDump(std::string_view data, size_t bytesPerLine) {
    const auto lines = FormatHexDumpLines(data, bytesPerLine);
    if (lines.empty()) {
        return {};
    }

    std::string result;
    for (size_t i = 0; i < lines.size(); ++i) {
        result += lines[i];
        if (i + 1 < lines.size()) {
            result += '\n';
        }
    }
    return result;
}

void TrafficPayloadFormatter::DrawHexDump(std::string_view data, size_t bytesPerLine) {
    if (data.empty()) {
        ImGui::TextDisabled("Empty payload");
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
    const auto lines = FormatHexDumpLines(data, bytesPerLine);
    for (const auto& line : lines) {
        ImGui::TextUnformatted(line.c_str());
    }
    ImGui::PopStyleColor();
}

}  // namespace GUI
