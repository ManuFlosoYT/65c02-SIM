#include "Frontend/GUI/Video/VRAMViewerWindow.h"

#include <ImGuiFileDialog.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <vector>

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static void DrawVRAMControls() {
    if (ImGui::Button("Load Image")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseVRAMImageKey", "Choose VRAM Image", ".bin", ".", "");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Capture VRAM")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            time_t currentTime = time(nullptr);
            struct tm* tmInfo = localtime(&currentTime);
            std::array<char, 64> filenameBuffer{};
            if (tmInfo != nullptr) {
                static_cast<void>(strftime(filenameBuffer.data(), filenameBuffer.size(),
                                           "65C02-SIM_VRAM_CAPTURE_%Y-%m-%d-%H-%M-%S.", tmInfo));
            }

            ImGuiFileDialog::Instance()->OpenDialog("SaveVRAMImageKey", "Save VRAM Image", ".bmp", ".",
                                                    filenameBuffer.data());
        }
    }

    ImGui::Separator();
}

static void UpdateVRAMTexture(AppState& state) {
    GPU& gpu = state.emulator.GetGPU();
    std::array<unsigned char, static_cast<size_t>(GPU::VRAM_HEIGHT) * static_cast<size_t>(GPU::VRAM_WIDTH) * 3>
        pixels{};
    for (size_t yIndex = 0; yIndex < GPU::VRAM_HEIGHT; yIndex++) {
        for (size_t xIndex = 0; xIndex < GPU::VRAM_WIDTH; xIndex++) {
            size_t idx = (yIndex * GPU::VRAM_WIDTH + xIndex) * 3;
            Byte val = gpu.GetVRAM().at(yIndex).at(xIndex);
            pixels.at(idx + 0) = ((val >> 4) & 0x03) * 85;
            pixels.at(idx + 1) = ((val >> 2) & 0x03) * 85;
            pixels.at(idx + 2) = (val & 0x03) * 85;
        }
    }
    glBindTexture(GL_TEXTURE_2D, state.vramTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GPU::VRAM_WIDTH, GPU::VRAM_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void LoadVRAMFromFile(const std::string& imgPath, AppState& state) {
    std::ifstream file(imgPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize > 0) {
        file.seekg(0, std::ios::beg);
        std::vector<unsigned char> buf(static_cast<size_t>(fileSize));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        if (file.read(reinterpret_cast<char*>(buf.data()), fileSize)) {
            GPU& gpu = state.emulator.GetGPU();
            for (size_t yIndex = 0; yIndex < GPU::VRAM_HEIGHT; yIndex++) {
                for (size_t xIndex = 0; xIndex < GPU::VRAM_WIDTH; xIndex++) {
                    size_t addr = (yIndex * 128) + xIndex;
                    if (addr < static_cast<size_t>(fileSize)) {
                        gpu.GetVRAM().at(yIndex).at(xIndex) = buf.at(addr);
                    }
                }
            }
        }
    }
}

static void HandleVRAMLoadDialog(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGuiFileDialog::Instance()->Display("ChooseVRAMImageKey")) {
        return;
    }

    if (ImGuiFileDialog::Instance()->IsOk()) {
        std::string imgPath = ImGuiFileDialog::Instance()->GetFilePathName();
        LoadVRAMFromFile(imgPath, state);
    }
    ImGuiFileDialog::Instance()->Close();
}

static void HandleVRAMSaveDialog(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGuiFileDialog::Instance()->Display("SaveVRAMImageKey")) {
        return;
    }

    if (ImGuiFileDialog::Instance()->IsOk()) {
        std::string imgPath = ImGuiFileDialog::Instance()->GetFilePathName();

        std::vector<unsigned char> pixels;
        int capW = 0;
        int capH = 0;

        if (state.lastDisplayTex != 0 && state.lastDisplayTex != state.vramTexture && state.lastDisplayW > 0) {
            capW = state.lastDisplayW;
            capH = state.lastDisplayH;
            pixels.resize(static_cast<std::size_t>(capW) * static_cast<std::size_t>(capH) * 3);
            glBindTexture(GL_TEXTURE_2D, state.lastDisplayTex);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        } else {
            capW = GPU::VRAM_WIDTH;
            capH = GPU::VRAM_HEIGHT;
            pixels.resize(static_cast<std::size_t>(capW) * static_cast<std::size_t>(capH) * 3);
            GPU& gpu = state.emulator.GetGPU();
            for (size_t yIndex = 0; yIndex < static_cast<size_t>(GPU::VRAM_HEIGHT); yIndex++) {
                for (size_t xIndex = 0; xIndex < static_cast<size_t>(GPU::VRAM_WIDTH); xIndex++) {
                    size_t idx = (yIndex * static_cast<size_t>(GPU::VRAM_WIDTH) + xIndex) * 3;
                    Byte val = gpu.GetVRAM().at(yIndex).at(xIndex);
                    pixels.at(idx + 0) = ((val >> 4) & 0x03) * 85;
                    pixels.at(idx + 1) = ((val >> 2) & 0x03) * 85;
                    pixels.at(idx + 2) = (val & 0x03) * 85;
                }
            }
        }

        SDL_Surface* surface = SDL_CreateSurfaceFrom(capW, capH, SDL_PIXELFORMAT_RGB24, pixels.data(), capW * 3);
        if (surface != nullptr) {
            SDL_SaveBMP(surface, imgPath.c_str());
            SDL_DestroySurface(surface);
        }
    }
    ImGuiFileDialog::Instance()->Close();
}

void DrawVRAMViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                          ImGuiWindowFlags window_flags) {
    float leftWidth = work_size.x * 0.5F;
    float rightWidth = work_size.x - leftWidth;
    float bottomSectionHeight = work_size.y - top_section_height;

    if (state.gpuEnabled) {
        ImGui::SetNextWindowPos(ImVec2(work_pos.x + leftWidth, work_pos.y + top_section_height), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(rightWidth, bottomSectionHeight), ImGuiCond_Always);
        ImGui::Begin("VRAM Viewer", nullptr, window_flags);

        DrawVRAMControls();
        UpdateVRAMTexture(state);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        int scaleX = static_cast<int>(avail.x / static_cast<float>(GPU::VRAM_WIDTH));
        int scaleY = static_cast<int>(avail.y / static_cast<float>(GPU::VRAM_HEIGHT));
        int scale = std::max(1, std::min(scaleX, scaleY));

        const auto imgW = static_cast<float>(GPU::VRAM_WIDTH * scale);
        const auto imgH = static_cast<float>(GPU::VRAM_HEIGHT * scale);
        float offsetX = (avail.x - imgW) * 0.5F;
        float offsetY = (avail.y - imgH) * 0.5F;
        if (offsetX > 0) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        }
        if (offsetY > 0) {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
        }

        bool anyCRT = state.crtScanlines || state.crtInterlacing || state.crtCurvature || state.crtChromatic ||
                      state.crtBlur || state.crtShadowMask || state.crtVignette || state.crtCornerRounding ||
                      state.crtGlassGlare || state.crtColorBleeding || state.crtNoise || state.crtVSyncJitter ||
                      state.crtPhosphorDecay || state.crtBloom;

        GLuint displayTex = state.vramTexture;
        if (anyCRT) {
            GUI::CRTParams params;
            params.scanlines = state.crtScanlines;
            params.interlacing = state.crtInterlacing;
            params.curvature = state.crtCurvature;
            params.chromatic = state.crtChromatic;
            params.blur = state.crtBlur;
            params.shadowMask = state.crtShadowMask;
            params.vignette = state.crtVignette;
            params.cornerRounding = state.crtCornerRounding;
            params.glassGlare = state.crtGlassGlare;
            params.colorBleeding = state.crtColorBleeding;
            params.noise = state.crtNoise;
            params.vsyncJitter = state.crtVSyncJitter;
            params.phosphorDecay = state.crtPhosphorDecay;
            params.bloom = state.crtBloom;
            params.time = state.crtTime;
            displayTex =
                state.crtFilter.Apply(state.vramTexture, static_cast<int>(imgW), static_cast<int>(imgH), params);
        }

        // NOLINTNEXTLINE(performance-no-int-to-ptr, cppcoreguidelines-pro-type-cstyle-cast)
        ImGui::Image((ImTextureID)(intptr_t)(displayTex), ImVec2(imgW, imgH));

        // Keep track of what was actually displayed for the capture button
        state.lastDisplayTex = displayTex;
        state.lastDisplayW = static_cast<int>(imgW);
        state.lastDisplayH = static_cast<int>(imgH);

        ImGui::End();
    }

    HandleVRAMLoadDialog(state);
    HandleVRAMSaveDialog(state);
}

}  // namespace GUI
