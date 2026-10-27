#include "Frontend/GUI/UIModules.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/GUI/ConsoleWindow.h"
#include "Frontend/GUI/ControlWindow.h"
#include "Frontend/GUI/LCDWindow.h"
#include "Frontend/GUI/ScriptConsoleWindow.h"
#include "Frontend/GUI/SIDViewerWindow.h"
#include "Frontend/GUI/UpdatePopup.h"
#include "Frontend/GUI/SDKUpdatePopup.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "Frontend/GUI/IDEWindow.h"
#include "Frontend/GUI/MIDI/MIDIWorkspaceWindow.h"

namespace Frontend {

void DrawGUIWindows(Control::AppState& state, const ImVec2& work_pos, const ImVec2& work_size, float top_section_height, unsigned int windowFlags) {
    GUI::DrawUpdatePopup(state);
#ifndef TARGET_WASM
    GUI::DrawSDKUpdatePopup(state);
#endif
    if (state.ide.open && state.ide.mode == Control::IDEMode::MIDI) {
        GUI::DrawSIDViewerWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
        GUI::DrawMIDIWorkspaceWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
        return;
    }
    GUI::DrawControlWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
    GUI::DrawLCDWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
    GUI::DrawSIDViewerWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
    GUI::DrawConsoleWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
    GUI::DrawScriptConsoleWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
    state.crt.time = static_cast<float>(SDL_GetTicks()) / 1000.0F;
    GUI::DrawVRAMViewerWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
    GUI::DrawIDEWindow(state);
}

}  // namespace Frontend
