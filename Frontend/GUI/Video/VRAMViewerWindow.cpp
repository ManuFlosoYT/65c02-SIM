#include "Frontend/GUI/Video/VRAMViewerWindow.h"

#include <ImGuiFileDialog.h>
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#include <fstream>
#endif

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <vector>

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

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

static void DrawVRAMControls(AppState& state) {
#ifdef TARGET_WASM
    bool cartLoaded = state.emulator.GetCartridge().loaded;
    ImGui::BeginDisabled(cartLoaded);
    if (ImGui::Button("Load Image")) {
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            std::string virtualPath = "/tmp/" + std::string(filename);
            FILE* f = fopen(virtualPath.c_str(), "wb");
            if (f) {
                fwrite(data, 1, size, f);
                fclose(f);
            }
            LoadVRAMFromFile(virtualPath, state);
        };
        WebFileUtils::open_browser_file_picker(".bin");
    }
    ImGui::EndDisabled();
    if (cartLoaded && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Eject cartridge first to load images to VRAM");
    }

    ImGui::SameLine();
    if (ImGui::Button("Capture VRAM")) {
        std::string tempPath = "/tmp/vram_capture.bmp";
        std::vector<unsigned char> pixels;
        int capW = GPU::VRAM_WIDTH;
        int capH = GPU::VRAM_HEIGHT;
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
        SDL_Surface* surface = SDL_CreateSurfaceFrom(capW, capH, SDL_PIXELFORMAT_RGB24, pixels.data(), capW * 3);
        if (surface != nullptr) {
            SDL_SaveBMP(surface, tempPath.c_str());
            SDL_DestroySurface(surface);
            std::ifstream ifs(tempPath, std::ios::binary);
            std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            WebFileUtils::download_file("vram_capture.bmp", buffer.data(), buffer.size());
        }
    }
#else
    bool cartLoaded = state.emulator.GetCartridge().loaded;
    ImGui::BeginDisabled(cartLoaded);
    if (ImGui::Button("Load Image")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseVRAMImageKey", "Choose VRAM Image", ".bin", ".", "");
        }
    }
    ImGui::EndDisabled();
    if (cartLoaded && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Eject cartridge first to load images to VRAM");
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
#endif

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
    glBindTexture(GL_TEXTURE_2D, state.render.vramTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GPU::VRAM_WIDTH, GPU::VRAM_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}


#ifndef TARGET_WASM
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

        if (state.render.lastDisplayTex != 0 && state.render.lastDisplayTex != state.render.vramTexture && state.render.lastDisplayW > 0) {
            capW = state.render.lastDisplayW;
            capH = state.render.lastDisplayH;
            pixels.resize(static_cast<std::size_t>(capW) * static_cast<std::size_t>(capH) * 3);
            glBindTexture(GL_TEXTURE_2D, state.render.lastDisplayTex);
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
#endif

void DrawVRAMViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                          ImGuiWindowFlags window_flags) {
    float leftWidth = work_size.x * 0.5F;
    float rightWidth = work_size.x - leftWidth;
    float bottomSectionHeight = work_size.y - top_section_height;

    if (state.emulation.gpuEnabled) {
        ImGui::SetNextWindowPos(ImVec2(work_pos.x + leftWidth, work_pos.y + top_section_height), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(rightWidth, bottomSectionHeight), ImGuiCond_Always);
        ImGui::Begin("VRAM Viewer", nullptr, window_flags);

        DrawVRAMControls(state);
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

        bool anyCRT = state.crt.scanlines || state.crt.interlacing || state.crt.curvature || state.crt.chromatic ||
                      state.crt.blur || state.crt.shadowMask || state.crt.vignette || state.crt.cornerRounding ||
                      state.crt.glassGlare || state.crt.colorBleeding || state.crt.noise || state.crt.vsyncJitter ||
                      state.crt.phosphorDecay || state.crt.bloom || state.crt.ghosting || state.crt.halation || 
                      state.crt.moire || state.crt.gamma != 2.2F;

        GLuint displayTex = state.render.vramTexture;
        if (anyCRT) {
            GUI::CRTParams params;
            params.scanlines = state.crt.scanlines;
            params.interlacing = state.crt.interlacing;
            params.curvature = state.crt.curvature;
            params.chromatic = state.crt.chromatic;
            params.blur = state.crt.blur;
            params.shadowMask = state.crt.shadowMask;
            params.vignette = state.crt.vignette;
            params.cornerRounding = state.crt.cornerRounding;
            params.glassGlare = state.crt.glassGlare;
            params.colorBleeding = state.crt.colorBleeding;
            params.noise = state.crt.noise;
            params.vsyncJitter = state.crt.vsyncJitter;
            params.phosphorDecay = state.crt.phosphorDecay;
            params.bloom = state.crt.bloom;
            params.ghosting = state.crt.ghosting;
            params.halation = state.crt.halation;
            params.moire = state.crt.moire;
            params.gamma = state.crt.gamma;
            params.time = state.crt.time;

            // We use a high internal resolution (1080p height) for better recording quality
            // while keeping the aspect ratio (100:75 -> 4:3 -> 1440:1080)
            const int internalH = 1080;
            const int internalW = 1440;

            displayTex = state.crtFilter.Apply(state.render.vramTexture, internalW, internalH, params);
            
            // Keep track of what was actually displayed for the capture button
            state.render.lastDisplayTex = displayTex;
            state.render.lastDisplayW = internalW;
            state.render.lastDisplayH = internalH;
        } else {
            // Keep track of raw VRAM texture
            state.render.lastDisplayTex = state.render.vramTexture;
            state.render.lastDisplayW = GPU::VRAM_WIDTH;
            state.render.lastDisplayH = GPU::VRAM_HEIGHT;
        }

        // NOLINTNEXTLINE(performance-no-int-to-ptr, cppcoreguidelines-pro-type-cstyle-cast)
        ImGui::Image((ImTextureID)(intptr_t)(displayTex), ImVec2(imgW, imgH));

        ImGui::End();
    }

#ifndef TARGET_WASM
    HandleVRAMLoadDialog(state);
    HandleVRAMSaveDialog(state);
#endif
}

}  // namespace GUI
