#include "Frontend/GUI/Debugger/ProfilerWindow.h"

#include <ImGuiFileDialog.h>
#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <imgui.h>

#include <cmath>
#include <cstdio>
#include <ctime>

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

    if (ImGui::IsItemHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 imgPos = ImGui::GetItemRectMin();
        ImVec2 imgSize = ImGui::GetItemRectSize();

        int memX = (int)((mousePos.x - imgPos.x) / imgSize.x * 256.0f);
        int memY = (int)((mousePos.y - imgPos.y) / imgSize.y * 256.0f);
        if (memX < 0) memX = 0;
        if (memX > 255) memX = 255;
        if (memY < 0) memY = 0;
        if (memY > 255) memY = 255;

        Word addr = (memY << 8) | memX;

        if (ImGui::BeginItemTooltip()) {
            ImGui::TextDisabled("Memory Inspector");
            ImGui::Separator();
            ImGui::Text("Address:  0x%04X", addr);
            ImGui::Text("Accesses: %u", counts[addr]);
            ImGui::EndTooltip();
        }
    }

    ImGui::SetCursorPos(ImVec2(10, 10));
    if (ImGui::Button("Clear Data")) {
        state.emulator.ClearProfiler();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export RAW")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            time_t t = time(nullptr);
            struct tm* tm_info = localtime(&t);
            char filenameBuffer[64];
            strftime(filenameBuffer, sizeof(filenameBuffer),
                     "65C02-SIM_PROFILER_RAW_%Y-%m-%d-%H-%M-%S.", tm_info);

            ImGuiFileDialog::Instance()->OpenDialog(
                "SaveProfilerRawKey", "Save Profiler Raw Data", ".txt", ".",
                filenameBuffer);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Export as BMP")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            time_t t = time(nullptr);
            struct tm* tm_info = localtime(&t);
            char filenameBuffer[64];
            strftime(filenameBuffer, sizeof(filenameBuffer),
                     "65C02-SIM_PROFILER_EXPORT_%Y-%m-%d-%H-%M-%S.", tm_info);

            ImGuiFileDialog::Instance()->OpenDialog(
                "SaveProfilerImageKey", "Save Profiler Heatmap", ".bmp", ".",
                filenameBuffer);
        }
    }

    // Profiler Image Save Dialog
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("SaveProfilerImageKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string imgPath =
                ImGuiFileDialog::Instance()->GetFilePathName();

            SDL_Surface* surface = SDL_CreateSurfaceFrom(
                256, 256, SDL_PIXELFORMAT_RGB24, pixels, 256 * 3);
            if (surface) {
                SDL_SaveBMP(surface, imgPath.c_str());
                SDL_DestroySurface(surface);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Profiler Raw Save Dialog
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("SaveProfilerRawKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath =
                ImGuiFileDialog::Instance()->GetFilePathName();

            FILE* f = fopen(filePath.c_str(), "w");
            if (f) {
                for (int i = 0; i < 65536; i++) {
                    fprintf(f, "%s0x%04X: %u", (i > 0 ? "\n" : ""), i, counts[i]);
                }
                fclose(f);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

}  // namespace GUI
