#include "Frontend/GUI/Debugger/ProfilerWindow.h"

#include <glad/gl.h>
#include <imgui.h>

#include <cmath>

using namespace Control;
using namespace Hardware;

namespace GUI {

void DrawProfilerWindow(AppState& state) {
    state.emulator.SetProfilingEnabled(true);
    uint32_t* counts = state.emulator.GetProfilerCounts();

    uint32_t maxCount = 0;
    for (int i = 0; i < 65536; i++) {
        if (counts[i] > maxCount) maxCount = counts[i];
    }

    static unsigned char pixels[256 * 256 * 3];
    if (maxCount > 0) {
        float logMax = std::log((float)maxCount + 1.0f);
        for (int i = 0; i < 65536; i++) {
            int idx = i * 3;
            float intensity = std::log((float)counts[i] + 1.0f) / logMax;
            pixels[idx + 0] = (unsigned char)(intensity * 255.0f);
            pixels[idx + 1] = 0;
            pixels[idx + 2] = 0;
        }
    } else {
        for (int i = 0; i < 65536 * 3; i++) pixels[i] = 0;
    }

    glBindTexture(GL_TEXTURE_2D, state.profilerTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE,
                    pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float size = (avail.x < avail.y ? avail.x : avail.y) * 0.9f;

    ImGui::SetCursorPos(
        ImVec2((avail.x - size) * 0.5f, (avail.y - size) * 0.5f));
    ImGui::Image((ImTextureID)(intptr_t)state.profilerTexture,
                 ImVec2(size, size));

    if (ImGui::BeginItemTooltip()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 imgPos = ImGui::GetItemRectMin();
        ImVec2 imgSize = ImGui::GetItemRectSize();
        int memX = (int)((mousePos.x - imgPos.x) / imgSize.x * 255.0f);
        int memY = (int)((mousePos.y - imgPos.y) / imgSize.y * 255.0f);
        if (memX >= 0 && memX < 256 && memY >= 0 && memY < 256) {
            Word addr = (memY << 8) | memX;
            ImGui::Text("Addr: 0x%04X", addr);
            ImGui::Text("Accesses: %u", counts[addr]);
        }
        ImGui::EndTooltip();
    }

    ImGui::SetCursorPos(ImVec2(10, 10));
    if (ImGui::Button("Clear Data")) {
        state.emulator.ClearProfiler();
    }
}

}  // namespace GUI
