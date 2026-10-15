#include "Frontend/GUI/ConsoleView.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <string_view>

#include "Frontend/Control/Console.h"

namespace GUI {

static void InjectSequence(Control::AppState& state, std::string_view seq) {
    for (char chr : seq) {
        state.emulator.InjectKey(chr);
    }
}

static void HandleQueueCharacters(Control::AppState& state, const ImGuiIO& imgui_io) {
    for (int idx = 0; idx < imgui_io.InputQueueCharacters.Size; idx++) {
        unsigned int chr = imgui_io.InputQueueCharacters[idx];
        if (chr == '\r' || chr == '\n') {
            continue;
        }
        if (chr < 0x80) {
            state.emulator.InjectKey(static_cast<char>(chr));
        } else if (chr < 0x800) {
            state.emulator.InjectKey(static_cast<char>(0xC0 | (chr >> 6)));
            state.emulator.InjectKey(static_cast<char>(0x80 | (chr & 0x3F)));
        } else if (chr < 0x10000) {
            state.emulator.InjectKey(static_cast<char>(0xE0 | (chr >> 12)));
            state.emulator.InjectKey(static_cast<char>(0x80 | ((chr >> 6) & 0x3F)));
            state.emulator.InjectKey(static_cast<char>(0x80 | (chr & 0x3F)));
        } else if (chr < 0x110000) {
            state.emulator.InjectKey(static_cast<char>(0xF0 | (chr >> 18)));
            state.emulator.InjectKey(static_cast<char>(0x80 | ((chr >> 12) & 0x3F)));
            state.emulator.InjectKey(static_cast<char>(0x80 | ((chr >> 6) & 0x3F)));
            state.emulator.InjectKey(static_cast<char>(0x80 | (chr & 0x3F)));
        }
    }
}

static void HandleNavigationKeys(Control::AppState& state, const ImGuiIO& imgui_io) {
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        InjectSequence(state, "\x1b[A");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        InjectSequence(state, "\x1b[B");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        InjectSequence(state, "\x1b[C");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        InjectSequence(state, "\x1b[D");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        InjectSequence(state, "\x1b[H");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        InjectSequence(state, "\x1b[F");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Insert)) {
        InjectSequence(state, "\x1b[2~");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        InjectSequence(state, "\x1b[3~");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
        InjectSequence(state, "\x1b[5~");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
        InjectSequence(state, "\x1b[6~");
    }
    if (imgui_io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        InjectSequence(state, "\x1b[Z");
    }
}

static void HandleSpecialKeys(Control::AppState& state, const ImGuiIO& imgui_io) {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        state.emulator.InjectKey(0x1B);
    }
    if (!imgui_io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        state.emulator.InjectKey(0x09);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        state.emulator.InjectKey(0x7F);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        state.emulator.InjectKey('\r');
    }
}

static void HandleCtrlKeys(Control::AppState& state, const ImGuiIO& imgui_io) {
    if (imgui_io.KeyCtrl) {
        for (int key = ImGuiKey_A; key <= ImGuiKey_Z; key++) {
            if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key))) {
                state.emulator.InjectKey(static_cast<char>(1 + (key - static_cast<int>(ImGuiKey_A))));
            }
        }
    }
}

void HandleConsoleInput(Control::AppState& state) {
    const ImGuiIO& imgui_io = ImGui::GetIO();
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        SDL_Window* sdl_window = SDL_GetKeyboardFocus();
        if (sdl_window != nullptr) {
            SDL_StartTextInput(sdl_window);
        }

        HandleQueueCharacters(state, imgui_io);
        HandleNavigationKeys(state, imgui_io);
        HandleSpecialKeys(state, imgui_io);
        HandleCtrlKeys(state, imgui_io);
    }
}

static void CodepointToUtf8(uint32_t cp, std::array<char, 5>& out) {
    out.fill(0);
    if (cp < 0x80) {
        out[0] = static_cast<char>(cp);
    } else if (cp < 0x800) {
        out[0] = static_cast<char>(0xC0 | (cp >> 6));
        out[1] = static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out[0] = static_cast<char>(0xE0 | (cp >> 12));
        out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x110000) {
        out[0] = static_cast<char>(0xF0 | (cp >> 18));
        out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (cp & 0x3F));
    }
}

void DrawConsoleView(Control::AppState& state) {
    HandleConsoleInput(state);

    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();
    float charWidth = ImGui::CalcTextSize("A").x;
    float charHeight = ImGui::GetTextLineHeight();

    float totalWidth = charWidth * static_cast<float>(Console::TERMINAL_COLS);
    float totalHeight = charHeight * static_cast<float>(Console::TERMINAL_ROWS);

    ImGui::BeginChild("TerminalRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(totalWidth, totalHeight));

    for (int row = 0; row < Console::TERMINAL_ROWS; ++row) {
        int col = 0;
        while (col < Console::TERMINAL_COLS) {
            const auto& firstCell = Console::GetCell(col, row);
            uint32_t bg = firstCell.bg;
            if ((firstCell.attributes & Console::ATTR_INVERSE) != 0) {
                bg = firstCell.fg;
            }

            int runEnd = col + 1;
            while (runEnd < Console::TERMINAL_COLS) {
                const auto& nextCell = Console::GetCell(runEnd, row);
                uint32_t nextBg = nextCell.bg;
                if ((nextCell.attributes & Console::ATTR_INVERSE) != 0) {
                    nextBg = nextCell.fg;
                }
                if (nextBg != bg) {
                    break;
                }
                runEnd++;
            }

            if ((bg & 0xFF000000) != 0) {
                ImVec2 rectMin(origin.x + static_cast<float>(col) * charWidth,
                               origin.y + static_cast<float>(row) * charHeight);
                ImVec2 rectMax(origin.x + static_cast<float>(runEnd) * charWidth,
                               origin.y + static_cast<float>(row + 1) * charHeight);
                drawList->AddRectFilled(rectMin, rectMax, bg);
            }

            col = runEnd;
        }

        for (col = 0; col < Console::TERMINAL_COLS; ++col) {
            const auto& cell = Console::GetCell(col, row);
            if (cell.codepoint > 32) {
                uint32_t fg = cell.fg;
                if ((cell.attributes & Console::ATTR_INVERSE) != 0) {
                    fg = cell.bg;
                    if ((fg & 0xFF000000) == 0) {
                        fg = 0xFF000000;
                    }
                }

                std::array<char, 5> utf8Buf = {0};
                CodepointToUtf8(cell.codepoint, utf8Buf);

                ImVec2 textPos(origin.x + static_cast<float>(col) * charWidth,
                               origin.y + static_cast<float>(row) * charHeight);
                drawList->AddText(font, fontSize, textPos, fg, utf8Buf.data());

                if ((cell.attributes & Console::ATTR_UNDERLINE) != 0) {
                    drawList->AddLine(ImVec2(textPos.x, textPos.y + charHeight - 1.0F),
                                      ImVec2(textPos.x + charWidth, textPos.y + charHeight - 1.0F), fg);
                }
            }
        }
    }

    if (Console::IsCursorVisible() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        auto blinkState = static_cast<int>(ImGui::GetTime() * 2.0);
        if ((blinkState % 2) == 0) {
            int cx = Console::GetCursorX();
            int cy = Console::GetCursorY();
            if (cx >= 0 && cx < Console::TERMINAL_COLS && cy >= 0 && cy < Console::TERMINAL_ROWS) {
                ImVec2 cMin(origin.x + static_cast<float>(cx) * charWidth,
                            origin.y + static_cast<float>(cy) * charHeight);
                ImVec2 cMax(origin.x + static_cast<float>(cx + 1) * charWidth,
                            origin.y + static_cast<float>(cy + 1) * charHeight);
                drawList->AddRect(cMin, cMax, Console::DEFAULT_FG);
            }
        }
    }

    ImGui::EndChild();
}

}  // namespace GUI
