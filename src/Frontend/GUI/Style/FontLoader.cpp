#include "Frontend/GUI/Style/FontLoader.h"
#include "Frontend/GUI/Style/CascadiaMono.h"

#include <imgui.h>

namespace GUI {

void FontLoader::SetupFont() {
    ImGuiIO& imguiio = ImGui::GetIO();

    static const ImWchar glyph_ranges[] = {
        0x0020, 0x00FF,
        0x2500, 0x257F,
        0x2580, 0x259F,
        0,
    };

    ImFont* mainFont = imguiio.Fonts->AddFontFromMemoryCompressedBase85TTF(
        GetCascadiaMonoCompressedBase85(),
        14.0F,
        nullptr,
        glyph_ranges
    );

    IM_ASSERT(mainFont != nullptr);
}

}  // namespace GUI
