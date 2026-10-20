#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include <imgui_internal.h>
#include <bit>

#include "Frontend/UI/CustomFileDialog.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#include <fstream>
#endif

#include <algorithm>
#include <array>
#include <cstdio>
#include <ctime>
#include <vector>

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

namespace {

int selectedPixelX = -1;
int selectedPixelY = -1;
bool hasPixelSelection = false;

Word GetVRAMBusAddress(AppState& state, int x, int y, bool& isMapped) {
    Word baseAddr = 0x2000;
    Word endAddr = 0x3FFF;
    isMapped = true;

    for (const auto& reg : state.emulator.GetMem().GetRegisteredDevices()) {
        if (reg.device == &state.emulator.GetGPU() && !reg.isVirtual) {
            baseAddr = reg.startAddress;
            endAddr = reg.endAddress;
            isMapped = reg.enabled;
            break;
        }
    }

    Word offset = static_cast<Word>((static_cast<Word>(y) << 7U) | static_cast<Word>(x));
    Word fullAddr = baseAddr + offset;
    if (fullAddr > endAddr) {
        isMapped = false;
    }
    return fullAddr;
}

ImVec4 VRAMColorToRGBA(Byte val) {
    float r = static_cast<float>((val >> 4) & 0x03) / 3.0F;
    float g = static_cast<float>((val >> 2) & 0x03) / 3.0F;
    float b = static_cast<float>(val & 0x03) / 3.0F;
    return {r, g, b, 1.0F};
}

}  // namespace

static void ConvertVRAMToRGB(const Hardware::GPU& gpu, uint8_t* dst) {
    for (size_t yIndex = 0; yIndex < GPU::VRAM_HEIGHT; yIndex++) {
        for (size_t xIndex = 0; xIndex < GPU::VRAM_WIDTH; xIndex++) {
            size_t idx = (yIndex * GPU::VRAM_WIDTH + xIndex) * 3;
            Byte val = gpu.GetVRAM().at(yIndex).at(xIndex);
            dst[idx + 0] = ((val >> 4) & 0x03) * 85;
            dst[idx + 1] = ((val >> 2) & 0x03) * 85;
            dst[idx + 2] = (val & 0x03) * 85;
        }
    }
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
        if (file.read(std::bit_cast<char*>(buf.data()), fileSize)) {
            GPU& gpu = state.emulator.GetGPU();
            gpu.LoadVRAM(buf);
        }
    }
}

static void DrawVRAMControls(AppState& state) {
#ifdef TARGET_WASM
    bool cartLoaded = state.emulator.GetCartridge().loaded;
    ImGui::BeginDisabled(cartLoaded || state.rom.loaded);
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
    if ((cartLoaded || state.rom.loaded) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Eject cartridge/ROM first to load images to VRAM");
            ImGui::EndTooltip();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Export VRAM")) {
        std::string tempPath = "/tmp/vram_capture.bmp";
        std::vector<unsigned char> pixels;
        int capW = GPU::VRAM_WIDTH;
        int capH = GPU::VRAM_HEIGHT;
        pixels.resize(static_cast<std::size_t>(capW) * static_cast<std::size_t>(capH) * 3);
        GPU& gpu = state.emulator.GetGPU();
        ConvertVRAMToRGB(gpu, pixels.data());
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
    ImGui::BeginDisabled(cartLoaded || state.rom.loaded);
    if (ImGui::Button("Load Image")) {
        if (!Frontend::CustomFileDialog::IsOpened()) {
            Frontend::CustomFileDialog::OpenDialog("ChooseVRAMImageKey", "Choose VRAM Image", ".bin", ".", "");
        }
    }
    ImGui::EndDisabled();
    if ((cartLoaded || state.rom.loaded) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Eject cartridge/ROM first to load images to VRAM");
            ImGui::EndTooltip();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Export VRAM")) {
        if (!Frontend::CustomFileDialog::IsOpened()) {
            time_t currentTime = time(nullptr);
            struct tm* tmInfo = localtime(&currentTime);
            std::array<char, 64> filenameBuffer{};
            if (tmInfo != nullptr) {
                static_cast<void>(strftime(filenameBuffer.data(), filenameBuffer.size(),
                                           "65C02-SIM_VRAM_CAPTURE_%Y-%m-%d-%H-%M-%S.", tmInfo));
            }

            Frontend::CustomFileDialog::OpenDialog("SaveVRAMImageKey", "Save VRAM Image", ".bmp", ".",
                                                    filenameBuffer.data());
        }
    }
#endif

    ImGui::SameLine();
    ImGui::BeginDisabled(!hasPixelSelection);
    if (ImGui::Button("Clear")) {
        hasPixelSelection = false;
    }
    ImGui::EndDisabled();

    if (hasPixelSelection && selectedPixelX >= 0 && selectedPixelX < GPU::VRAM_WIDTH && selectedPixelY >= 0 &&
        selectedPixelY < GPU::VRAM_HEIGHT) {
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

        Byte val = state.emulator.GetGPU().GetVRAM().at(static_cast<size_t>(selectedPixelY)).at(static_cast<size_t>(selectedPixelX));
        ImVec4 col = VRAMColorToRGBA(val);

        bool isMapped = false;
        Word busAddr = GetVRAMBusAddress(state, selectedPixelX, selectedPixelY, isMapped);

        ImGui::SameLine();
        if (isMapped) {
            ImGui::Text("$%04X (%02d, %02d)", busAddr, selectedPixelX, selectedPixelY);
        } else {
            ImGui::Text("$%04X (%02d, %02d) [Unmapped]", busAddr, selectedPixelX, selectedPixelY);
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

        ImGui::SameLine();
        ImGui::Text("RGB: 0x%02X", static_cast<unsigned int>(val & 0x3FU));
        ImGui::SameLine();
        ImGui::ColorButton("##selected_color", col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs,
                           ImVec2(14, 14));
    }

    ImGui::Separator();
}

void ForceRefreshVRAM(AppState& state) {
    GPU& gpu = state.emulator.GetGPU();
    std::array<unsigned char, static_cast<size_t>(GPU::VRAM_HEIGHT) * static_cast<size_t>(GPU::VRAM_WIDTH) * 3>
        pixels{};
    ConvertVRAMToRGB(gpu, pixels.data());
    glBindTexture(GL_TEXTURE_2D, state.render.vramTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GPU::VRAM_WIDTH, GPU::VRAM_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}


#ifndef TARGET_WASM
static void HandleVRAMLoadDialog(AppState& state) {
    if (!Frontend::CustomFileDialog::Display("ChooseVRAMImageKey")) {
        return;
    }

    if (Frontend::CustomFileDialog::IsOk()) {
        std::string imgPath = Frontend::CustomFileDialog::GetFilePathName();
        LoadVRAMFromFile(imgPath, state);
    }
    Frontend::CustomFileDialog::Close();
}

static void HandleVRAMSaveDialog(AppState& state) {
    if (!Frontend::CustomFileDialog::Display("SaveVRAMImageKey")) {
        return;
    }

    if (Frontend::CustomFileDialog::IsOk()) {
        std::string imgPath = Frontend::CustomFileDialog::GetFilePathName();

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
            ConvertVRAMToRGB(gpu, pixels.data());
        }

        SDL_Surface* surface = SDL_CreateSurfaceFrom(capW, capH, SDL_PIXELFORMAT_RGB24, pixels.data(), capW * 3);
        if (surface != nullptr) {
            SDL_SaveBMP(surface, imgPath.c_str());
            SDL_DestroySurface(surface);
        }
    }
    Frontend::CustomFileDialog::Close();
}
#endif

static void DrawRasterBeamOverlay(AppState& state, ImVec2 imgMin, int scale, float imgW, float imgH) {
    const GPU& gpu = state.emulator.GetGPU();
    uint16_t beamX = gpu.GetPixelX();
    uint16_t beamY = gpu.GetPixelY();
    bool isBlanking = gpu.IsInBlankingInterval();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float scaleF = static_cast<float>(scale);

    if (beamX < GPU::VRAM_WIDTH && beamY < GPU::VRAM_HEIGHT) {
        float px = imgMin.x + static_cast<float>(beamX) * scaleF;
        float py = imgMin.y + static_cast<float>(beamY) * scaleF;

        drawList->AddLine(ImVec2(imgMin.x, py + scaleF * 0.5F),
                          ImVec2(imgMin.x + imgW, py + scaleF * 0.5F),
                          IM_COL32(0, 255, 180, 45), 1.0F);

        drawList->AddLine(ImVec2(imgMin.x, py + scaleF * 0.5F),
                          ImVec2(px, py + scaleF * 0.5F),
                          IM_COL32(50, 255, 160, 120), 1.5F);

        ImVec2 beamCenter(px + scaleF * 0.5F, py + scaleF * 0.5F);
        drawList->AddCircleFilled(beamCenter, scaleF * 2.5F, IM_COL32(0, 230, 255, 45));
        drawList->AddCircleFilled(beamCenter, scaleF * 1.3F, IM_COL32(80, 255, 200, 110));

        drawList->AddRectFilled(ImVec2(px, py), ImVec2(px + scaleF, py + scaleF),
                                IM_COL32(255, 255, 255, 240));
        drawList->AddRect(ImVec2(px - 1.0F, py - 1.0F), ImVec2(px + scaleF + 1.0F, py + scaleF + 1.0F),
                          IM_COL32(0, 255, 255, 255), 0.0F, 0, 1.5F);
    } else if (beamY < GPU::VRAM_HEIGHT) {
        float py = imgMin.y + static_cast<float>(beamY) * scaleF;
        float hblankSpan = static_cast<float>(GPU::DISPLAY_WIDTH - GPU::VRAM_WIDTH);
        float hblankProg = static_cast<float>(beamX - GPU::VRAM_WIDTH) / hblankSpan;
        float retraceX = (imgMin.x + imgW) - (hblankProg * imgW);
        drawList->AddLine(ImVec2(imgMin.x + imgW, py + scaleF * 0.5F),
                          ImVec2(imgMin.x, py + scaleF * 1.5F),
                          IM_COL32(255, 170, 0, 60), 1.0F);
        drawList->AddCircleFilled(ImVec2(retraceX, py + scaleF * (0.5F + hblankProg)),
                                  3.0F, IM_COL32(255, 180, 40, 170));
    } else {
        float vblankSpan = static_cast<float>(GPU::DISPLAY_HEIGHT - GPU::VRAM_HEIGHT);
        float vblankProg = static_cast<float>(beamY - GPU::VRAM_HEIGHT) / vblankSpan;
        drawList->AddLine(ImVec2(imgMin.x + imgW, imgMin.y + imgH),
                          ImVec2(imgMin.x, imgMin.y),
                          IM_COL32(255, 70, 70, 60), 1.0F);
        drawList->AddCircleFilled(ImVec2((imgMin.x + imgW) - (vblankProg * imgW),
                                         (imgMin.y + imgH) - (vblankProg * imgH)),
                                  4.0F, IM_COL32(255, 90, 90, 180));
    }

    std::array<char, 64> hudText{};
    const char* status = isBlanking ? (beamY >= GPU::VRAM_HEIGHT ? "V-BLANK" : "H-BLANK") : "DRAWING";
    std::snprintf(hudText.data(), hudText.size(), "BEAM: (%u, %u) [%s]", beamX, beamY, status);

    ImVec2 textPos(imgMin.x + 8.0F, imgMin.y + 8.0F);
    ImVec2 textSize = ImGui::CalcTextSize(hudText.data());
    drawList->AddRectFilled(ImVec2(textPos.x - 4.0F, textPos.y - 2.0F),
                            ImVec2(textPos.x + textSize.x + 4.0F, textPos.y + textSize.y + 2.0F),
                            IM_COL32(10, 15, 25, 200), 3.0F);
    uint32_t textColor = isBlanking ? IM_COL32(255, 180, 50, 255) : IM_COL32(80, 255, 180, 255);
    drawList->AddText(textPos, textColor, hudText.data());
}

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
        if (state.emulator.GetGPU().ConsumeVRAMDirty()) {
            ForceRefreshVRAM(state);
        }

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

        ImGui::Image((ImTextureID)(intptr_t)displayTex, ImVec2(imgW, imgH));
        ImVec2 imgMin = ImGui::GetItemRectMin();

        if (ImGui::IsItemHovered()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            int hoveredX = std::clamp(static_cast<int>((mousePos.x - imgMin.x) / imgW * static_cast<float>(GPU::VRAM_WIDTH)),
                                      0, static_cast<int>(GPU::VRAM_WIDTH - 1));
            int hoveredY = std::clamp(static_cast<int>((mousePos.y - imgMin.y) / imgH * static_cast<float>(GPU::VRAM_HEIGHT)),
                                      0, static_cast<int>(GPU::VRAM_HEIGHT - 1));

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                selectedPixelX = hoveredX;
                selectedPixelY = hoveredY;
                hasPixelSelection = true;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                hasPixelSelection = false;
            }

            if (ImGui::BeginItemTooltip()) {
                Byte val = state.emulator.GetGPU().GetVRAM().at(static_cast<size_t>(hoveredY)).at(static_cast<size_t>(hoveredX));
                uint8_t r = (val >> 4) & 0x03;
                uint8_t g = (val >> 2) & 0x03;
                uint8_t b = val & 0x03;
                ImVec4 col = VRAMColorToRGBA(val);

                bool isMapped = false;
                Word busAddr = GetVRAMBusAddress(state, hoveredX, hoveredY, isMapped);

                ImGui::Text("Pixel: (%d, %d)", hoveredX, hoveredY);
                if (isMapped) {
                    ImGui::Text("Bus: $%04X", busAddr);
                } else {
                    ImGui::Text("Bus: $%04X (Unmapped)", busAddr);
                }
                ImGui::Text("Palette: 0x%02X (%u) [R:%u G:%u B:%u]", static_cast<unsigned int>(val & 0x3FU),
                            static_cast<unsigned int>(val & 0x3FU), r, g, b);
                ImGui::SameLine();
                ImGui::ColorButton("##hover_color", col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs,
                                   ImVec2(14, 14));
                ImGui::EndTooltip();
            }
        }

        if (hasPixelSelection && selectedPixelX >= 0 && selectedPixelX < GPU::VRAM_WIDTH && selectedPixelY >= 0 &&
            selectedPixelY < GPU::VRAM_HEIGHT) {
            float scaleF = static_cast<float>(scale);
            float px = imgMin.x + static_cast<float>(selectedPixelX) * scaleF;
            float py = imgMin.y + static_cast<float>(selectedPixelY) * scaleF;
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRect(ImVec2(px, py), ImVec2(px + scaleF, py + scaleF),
                              IM_COL32(255, 255, 0, 255), 0.0F, 0, 1.5F);
        }

        if (state.crt.rasterBeam) {
            DrawRasterBeamOverlay(state, imgMin, scale, imgW, imgH);
        }

        ImGui::End();
    }

#ifndef TARGET_WASM
    HandleVRAMLoadDialog(state);
    HandleVRAMSaveDialog(state);
#endif
}

}  // namespace GUI
