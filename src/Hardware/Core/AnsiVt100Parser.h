#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Console {

struct TerminalBuffer;

enum class ParserState { Ground, Escape, Csi, CharsetG0, CharsetG1, Osc };

class AnsiVt100Parser {
   public:
    AnsiVt100Parser() = default;

    void Reset();
    void ProcessChar(char character, TerminalBuffer& buffer);
    void SetResponseCallback(std::function<void(char)> callback);

   private:
    ParserState parserState = ParserState::Ground;
    std::string oscBuffer;
    std::vector<int> csiParams;
    int currentParam = 0;
    bool hasCurrentParam = false;
    bool isPrivateMode = false;

    uint32_t utf8Codepoint = 0;
    int utf8Remaining = 0;
    bool decSpecialGraphics = false;

    std::function<void(char)> responseCallback;

    void ExecuteOsc(TerminalBuffer& buffer);
    void HandleSGR(TerminalBuffer& buffer);
    void ExecuteCsi(char cmd, TerminalBuffer& buffer);
};

}  // namespace Console
