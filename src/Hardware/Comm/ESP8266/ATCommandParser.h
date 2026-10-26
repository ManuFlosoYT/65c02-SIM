#pragma once

#include <cstddef>
#include <string>

namespace Hardware {

class ATCommandParser {
public:
    static inline std::string ParseQuotedParam(const std::string& cmd, size_t& pos) {
        size_t firstQuote = cmd.find('"', pos);
        if (firstQuote == std::string::npos) {
            return "";
        }
        size_t lastQuote = cmd.find('"', firstQuote + 1);
        if (lastQuote == std::string::npos) {
            return "";
        }
        pos = lastQuote + 1;
        return cmd.substr(firstQuote + 1, lastQuote - firstQuote - 1);
    }

    static inline int ParseIntParam(const std::string& cmd, size_t& pos) {
        size_t nextComma = cmd.find(',', pos);
        std::string valStr = cmd.substr(pos, nextComma - pos);
        if (nextComma != std::string::npos) {
            pos = nextComma;
        } else {
            pos = cmd.size();
        }
        try {
            return std::stoi(valStr);
        } catch (...) {
            return -1;
        }
    }
};

}  // namespace Hardware
