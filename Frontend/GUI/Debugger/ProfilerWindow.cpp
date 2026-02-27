#include "Frontend/GUI/Debugger/ProfilerWindow.h"

#include <ImGuiFileDialog.h>
#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <span>
#include <sstream>

using namespace Control;
using namespace Hardware;

namespace GUI {

namespace {

void UpdateProfilerTexture(AppState& state, std::span<const uint32_t> counts,
                           std::array<unsigned char, 256ULL * 256ULL * 3ULL>& pixels) {
    uint32_t maxCount = 0;
    for (uint32_t count : counts) {
        maxCount = std::max(maxCount, count);
    }

    if (maxCount > 0) {
        float logMax = std::log(static_cast<float>(maxCount) + 1.0F);
        for (std::size_t i = 0; i < counts.size(); i++) {
            std::size_t idx = i * 3U;
            float intensity = std::log(static_cast<float>(counts[i]) + 1.0F) / logMax;
            pixels.at(idx + 0) = static_cast<unsigned char>(intensity * 255.0F);
            pixels.at(idx + 1) = 0;
            pixels.at(idx + 2) = 0;
        }
    } else {
        pixels.fill(0);
    }

    glBindTexture(GL_TEXTURE_2D, state.profilerTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawProfilerTooltip(std::span<const uint32_t> counts, float imgSizeX, float imgSizeY) {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 imgPos = ImGui::GetItemRectMin();

    int memX = static_cast<int>((mousePos.x - imgPos.x) / imgSizeX * 256.0F);
    int memY = static_cast<int>((mousePos.y - imgPos.y) / imgSizeY * 256.0F);
    memX = std::clamp(memX, 0, 255);
    memY = std::clamp(memY, 0, 255);

    Word addr = static_cast<Word>((memY << 8) | memX);

    if (ImGui::BeginItemTooltip()) {
        ImGui::TextUnformatted("Memory Inspector");
        ImGui::Separator();

        std::ostringstream addrBuf;
        addrBuf << "Address:  0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << addr;
        ImGui::TextUnformatted(addrBuf.str().c_str());

        std::ostringstream countBuf;
        countBuf << "Accesses: " << std::dec << counts[addr];
        ImGui::TextUnformatted(countBuf.str().c_str());
        ImGui::EndTooltip();
    }
}

void DrawProfilerButtons(AppState& state) {
    ImGui::SetCursorPos(ImVec2(10, 5));
    if (ImGui::Button("Clear Data")) {
        state.emulator.ClearProfiler();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export RAW")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            time_t rawTime = time(nullptr);
            struct tm* tm_info = localtime(&rawTime);
            std::array<char, 64> filenameBuffer{};
            if (strftime(filenameBuffer.data(), filenameBuffer.size(), "65C02-SIM_PROFILER_RAW_%Y-%m-%d-%H-%M-%S.",
                         tm_info) > 0) {
                ImGuiFileDialog::Instance()->OpenDialog("SaveProfilerRawKey", "Save Profiler Raw Data", ".txt", ".",
                                                        filenameBuffer.data());
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Export as BMP")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            time_t rawTime = time(nullptr);
            struct tm* tm_info = localtime(&rawTime);
            std::array<char, 64> filenameBuffer{};
            if (strftime(filenameBuffer.data(), filenameBuffer.size(), "65C02-SIM_PROFILER_EXPORT_%Y-%m-%d-%H-%M-%S.",
                         tm_info) > 0) {
                ImGuiFileDialog::Instance()->OpenDialog("SaveProfilerImageKey", "Save Profiler Heatmap", ".bmp", ".",
                                                        filenameBuffer.data());
            }
        }
    }
}

void HandleProfilerDialogs(std::span<const uint32_t> counts, std::span<unsigned char> pixels) {
    // Profiler Image Save Dialog
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("SaveProfilerImageKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string imgPath = ImGuiFileDialog::Instance()->GetFilePathName();

            SDL_Surface* surface = SDL_CreateSurfaceFrom(256, 256, SDL_PIXELFORMAT_RGB24, pixels.data(), 256 * 3);
            if (surface != nullptr) {
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
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

            std::ofstream outFile(filePath);
            if (outFile.is_open()) {
                for (std::size_t i = 0; i < counts.size(); i++) {
                    if (i > 0) {
                        outFile << '\n';
                    }
                    outFile << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << i << ": "
                            << std::dec << counts[i];
                }
                outFile.close();
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

}  // namespace

void DrawProfilerWindow(AppState& state) {
    std::span<const uint32_t> counts(state.emulator.GetProfilerCounts(), 65536);
    static std::array<unsigned char, 256ULL * 256ULL * 3ULL> pixels{};

    UpdateProfilerTexture(state, counts, pixels);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float size = std::min(avail.x, avail.y) * 0.9F;

    ImGui::SetCursorPos(ImVec2((avail.x - size) * 0.5F, (avail.y - size) * 0.5F));
    ImGui::Image((ImTextureID)(intptr_t)state.profilerTexture, ImVec2(size, size));

    if (ImGui::IsItemHovered()) {
        DrawProfilerTooltip(counts, size, size);
    }

    DrawProfilerButtons(state);
    HandleProfilerDialogs(counts, pixels);
}

}  // namespace GUI
