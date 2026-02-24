#include "Frontend/GUI/Video/VRAMViewerWindow.h"

#include <ImGuiFileDialog.h>

#include <cstdio>
#include <ctime>
#include <vector>

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawVRAMViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size,
                          float top_section_height,
                          ImGuiWindowFlags window_flags) {
    float left_width = work_size.x * 0.5f;
    float right_width = work_size.x - left_width;
    float bottom_section_height = work_size.y - top_section_height;

    if (state.gpuEnabled) {
        ImGui::SetNextWindowPos(
            ImVec2(work_pos.x + left_width, work_pos.y + top_section_height),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(right_width, bottom_section_height),
                                 ImGuiCond_Always);
        ImGui::Begin("VRAM Viewer", nullptr, window_flags);

        if (ImGui::Button("Load Image")) {
            if (!ImGuiFileDialog::Instance()->IsOpened()) {
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseVRAMImageKey", "Choose VRAM Image", ".bin", ".", "");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Capture VRAM")) {
            if (!ImGuiFileDialog::Instance()->IsOpened()) {
                time_t t = time(nullptr);
                struct tm* tm_info = localtime(&t);
                char filenameBuffer[64];
                strftime(filenameBuffer, sizeof(filenameBuffer),
                         "65C02-SIM_VRAM_CAPTURE_%Y-%m-%d-%H-%M-%S.", tm_info);

                ImGuiFileDialog::Instance()->OpenDialog(
                    "SaveVRAMImageKey", "Save VRAM Image", ".bmp", ".",
                    filenameBuffer);
            }
        }

        ImGui::Separator();

        GPU& gpu = state.emulator.GetGPU();
        unsigned char pixels[GPU::VRAM_HEIGHT * GPU::VRAM_WIDTH * 3];
        for (int y = 0; y < GPU::VRAM_HEIGHT; y++) {
            for (int x = 0; x < GPU::VRAM_WIDTH; x++) {
                int idx = (y * GPU::VRAM_WIDTH + x) * 3;
                Byte val = gpu.vram[y][x];
                pixels[idx + 0] = ((val >> 4) & 0x03) * 85;
                pixels[idx + 1] = ((val >> 2) & 0x03) * 85;
                pixels[idx + 2] = (val & 0x03) * 85;
            }
        }
        glBindTexture(GL_TEXTURE_2D, state.vramTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GPU::VRAM_WIDTH,
                        GPU::VRAM_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        int scaleX = (int)(avail.x / GPU::VRAM_WIDTH);
        int scaleY = (int)(avail.y / GPU::VRAM_HEIGHT);
        int scale = scaleX < scaleY ? scaleX : scaleY;
        if (scale < 1) scale = 1;

        float imgW = (float)(GPU::VRAM_WIDTH * scale);
        float imgH = (float)(GPU::VRAM_HEIGHT * scale);
        float offsetX = (avail.x - imgW) * 0.5f;
        float offsetY = (avail.y - imgH) * 0.5f;
        if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        if (offsetY > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

        bool anyCRT =
            state.crtScanlines || state.crtInterlacing || state.crtCurvature ||
            state.crtChromatic || state.crtBlur || state.crtShadowMask ||
            state.crtVignette || state.crtCornerRounding ||
            state.crtGlassGlare || state.crtColorBleeding || state.crtNoise ||
            state.crtVSyncJitter || state.crtPhosphorDecay || state.crtBloom;

        GLuint displayTex = state.vramTexture;
        if (anyCRT) {
            GUI::CRTParams p;
            p.scanlines = state.crtScanlines;
            p.interlacing = state.crtInterlacing;
            p.curvature = state.crtCurvature;
            p.chromatic = state.crtChromatic;
            p.blur = state.crtBlur;
            p.shadowMask = state.crtShadowMask;
            p.vignette = state.crtVignette;
            p.cornerRounding = state.crtCornerRounding;
            p.glassGlare = state.crtGlassGlare;
            p.colorBleeding = state.crtColorBleeding;
            p.noise = state.crtNoise;
            p.vsyncJitter = state.crtVSyncJitter;
            p.phosphorDecay = state.crtPhosphorDecay;
            p.bloom = state.crtBloom;
            p.time = state.crtTime;
            displayTex = state.crtFilter.Apply(state.vramTexture, (int)imgW,
                                               (int)imgH, p);
        }

        ImGui::Image((ImTextureID)(intptr_t)displayTex, ImVec2(imgW, imgH));

        // Keep track of what was actually displayed for the capture button
        state.lastDisplayTex = displayTex;
        state.lastDisplayW = (int)imgW;
        state.lastDisplayH = (int)imgH;

        ImGui::End();
    }

    // VRAM Image Load Dialog
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("ChooseVRAMImageKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string imgPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            FILE* f = fopen(imgPath.c_str(), "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long fileSize = ftell(f);
                fseek(f, 0, SEEK_SET);
                unsigned char* buf = new unsigned char[fileSize];
                fread(buf, 1, fileSize, f);
                fclose(f);
                GPU& gpu = state.emulator.GetGPU();
                for (int y = 0; y < GPU::VRAM_HEIGHT; y++) {
                    for (int x = 0; x < GPU::VRAM_WIDTH; x++) {
                        size_t addr = (size_t)y * 128 + x;
                        if (addr < (size_t)fileSize) gpu.vram[y][x] = buf[addr];
                    }
                }
                delete[] buf;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // VRAM Image Save Dialog
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("SaveVRAMImageKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string imgPath =
                ImGuiFileDialog::Instance()->GetFilePathName();

            // If CRT filters are active read back the post-processed texture;
            // otherwise fall back to raw VRAM bytes so the output matches
            // exactly what was shown on screen.
            std::vector<unsigned char> pixels;
            int capW, capH;

            // Only read back via GL when the CRT FBO texture was rendered —
            // i.e. when the display texture is NOT the raw vramTexture.
            if (state.lastDisplayTex != 0 &&
                state.lastDisplayTex != state.vramTexture &&
                state.lastDisplayW > 0) {
                capW = state.lastDisplayW;
                capH = state.lastDisplayH;
                pixels.resize(capW * capH * 3);
                glBindTexture(GL_TEXTURE_2D, state.lastDisplayTex);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE,
                              pixels.data());
                glBindTexture(GL_TEXTURE_2D, 0);
            } else {
                capW = GPU::VRAM_WIDTH;
                capH = GPU::VRAM_HEIGHT;
                pixels.resize(capW * capH * 3);
                GPU& gpu = state.emulator.GetGPU();
                for (int y = 0; y < GPU::VRAM_HEIGHT; y++) {
                    for (int x = 0; x < GPU::VRAM_WIDTH; x++) {
                        int idx = (y * GPU::VRAM_WIDTH + x) * 3;
                        Byte val = gpu.vram[y][x];
                        pixels[idx + 0] = ((val >> 4) & 0x03) * 85;
                        pixels[idx + 1] = ((val >> 2) & 0x03) * 85;
                        pixels[idx + 2] = (val & 0x03) * 85;
                    }
                }
            }

            SDL_Surface* surface = SDL_CreateSurfaceFrom(
                capW, capH, SDL_PIXELFORMAT_RGB24, pixels.data(), capW * 3);
            if (surface) {
                SDL_SaveBMP(surface, imgPath.c_str());
                SDL_DestroySurface(surface);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

}  // namespace GUI
