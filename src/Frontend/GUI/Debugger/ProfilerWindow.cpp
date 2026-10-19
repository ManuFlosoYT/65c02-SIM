#include "Frontend/GUI/Debugger/ProfilerWindow.h"
#include "Frontend/UI/CustomFileDialog.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif
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

void UpdateProfilerTexture(AppState& state, std::span<const uint32_t> readCounts,
                           std::span<const uint32_t> writeCounts,
                           std::array<unsigned char, 256ULL * 256ULL * 3ULL>& pixels) {
    uint32_t maxAccess = 0;
    for (std::size_t i = 0; i < readCounts.size(); i++) {
        maxAccess = std::max({maxAccess, readCounts[i], writeCounts[i]});
    }

    if (maxAccess > 0) {
        float logMax = std::log(static_cast<float>(maxAccess) + 1.0F);
        for (std::size_t i = 0; i < readCounts.size(); i++) {
            std::size_t idx = i * 3U;
            float rIntensity = std::log(static_cast<float>(writeCounts[i]) + 1.0F) / logMax;
            float bIntensity = std::log(static_cast<float>(readCounts[i]) + 1.0F) / logMax;
            pixels.at(idx + 0) = static_cast<unsigned char>(rIntensity * 255.0F);
            pixels.at(idx + 1) = 0;
            pixels.at(idx + 2) = static_cast<unsigned char>(bIntensity * 255.0F);
        }
    } else {
        pixels.fill(0);
    }

    glBindTexture(GL_TEXTURE_2D, state.render.profilerTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawProfilerTooltip(std::span<const uint32_t> counts, std::span<const uint32_t> readCounts,
                         std::span<const uint32_t> writeCounts, float imgSizeX, float imgSizeY) {
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

        std::ostringstream readBuf;
        readBuf << "Reads:    " << std::dec << readCounts[addr];
        ImGui::TextColored(ImVec4(0.3F, 0.6F, 1.0F, 1.0F), "%s", readBuf.str().c_str());

        std::ostringstream writeBuf;
        writeBuf << "Writes:   " << std::dec << writeCounts[addr];
        ImGui::TextColored(ImVec4(1.0F, 0.3F, 0.3F, 1.0F), "%s", writeBuf.str().c_str());

        ImGui::EndTooltip();
    }
}

std::string GenerateProfilerRawData(std::span<const uint32_t> readCounts, std::span<const uint32_t> writeCounts) {
    std::ostringstream oss;
    oss << "DIR: R/W\n";
    for (std::size_t i = 0; i < readCounts.size(); i++) {
        uint32_t r = readCounts[i];
        uint32_t w = writeCounts[i];
        if (r > 0 || w > 0) {
            oss << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << i
                << ": " << std::dec << r << '/' << w << '\n';
        }
    }
    return oss.str();
}

void DrawProfilerButtons(AppState& state, std::span<const uint32_t> readCounts,
                         std::span<const uint32_t> writeCounts, std::span<unsigned char> pixels,
                         bool& profilerDirty) {
    ImGui::SetCursorPos(ImVec2(10, 5));
    if (ImGui::Button("Clear Data")) {
        state.emulator.ClearProfiler();
        profilerDirty = true;
    }
    ImGui::SameLine();
#ifdef TARGET_WASM
    if (ImGui::Button("Export RAW")) {
        std::string raw = GenerateProfilerRawData(readCounts, writeCounts);
        WebFileUtils::download_file("profiler_raw.txt", std::bit_cast<const uint8_t*>(raw.data()), raw.size());
    }
    ImGui::SameLine();
    if (ImGui::Button("Export as BMP")) {
        std::string tempPath = "/tmp/profiler.bmp";
        SDL_Surface* surface = SDL_CreateSurfaceFrom(256, 256, SDL_PIXELFORMAT_RGB24, const_cast<unsigned char*>(pixels.data()), 256 * 3);
        if (surface != nullptr) {
            SDL_SaveBMP(surface, tempPath.c_str());
            SDL_DestroySurface(surface);
            
            std::ifstream ifs(tempPath, std::ios::binary);
            std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            WebFileUtils::download_file("profiler_heatmap.bmp", buffer.data(), buffer.size());
        }
    }
#else
    (void)readCounts;
    (void)writeCounts;
    (void)pixels;
    if (ImGui::Button("Export RAW")) {
        if (!Frontend::CustomFileDialog::IsOpened()) {
            time_t rawTime = time(nullptr);
            struct tm* tm_info = localtime(&rawTime);
            std::array<char, 64> filenameBuffer{};
            if (strftime(filenameBuffer.data(), filenameBuffer.size(), "65C02-SIM_PROFILER_RAW_%Y-%m-%d-%H-%M-%S.",
                         tm_info) > 0) {
                Frontend::CustomFileDialog::OpenDialog("SaveProfilerRawKey", "Save Profiler Raw Data", ".txt", ".",
                                                        filenameBuffer.data());
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Export as BMP")) {
        if (!Frontend::CustomFileDialog::IsOpened()) {
            time_t rawTime = time(nullptr);
            struct tm* tm_info = localtime(&rawTime);
            std::array<char, 64> filenameBuffer{};
            if (strftime(filenameBuffer.data(), filenameBuffer.size(), "65C02-SIM_PROFILER_EXPORT_%Y-%m-%d-%H-%M-%S.",
                         tm_info) > 0) {
                Frontend::CustomFileDialog::OpenDialog("SaveProfilerImageKey", "Save Profiler Heatmap", ".bmp", ".",
                                                        filenameBuffer.data());
            }
        }
    }
#endif
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0F, 0.3F, 0.3F, 1.0F), "Write (Red)");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3F, 0.6F, 1.0F, 1.0F), "Read (Blue)");
}

#ifndef TARGET_WASM
void HandleProfilerDialogs(std::span<const uint32_t> readCounts, std::span<const uint32_t> writeCounts,
                           std::span<unsigned char> pixels) {
    // Profiler Image Save Dialog
    if (Frontend::CustomFileDialog::Display("SaveProfilerImageKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string imgPath = Frontend::CustomFileDialog::GetFilePathName();

            SDL_Surface* surface = SDL_CreateSurfaceFrom(256, 256, SDL_PIXELFORMAT_RGB24, pixels.data(), 256 * 3);
            if (surface != nullptr) {
                SDL_SaveBMP(surface, imgPath.c_str());
                SDL_DestroySurface(surface);
            }
        }
        Frontend::CustomFileDialog::Close();
    }

    // Profiler Raw Save Dialog
    if (Frontend::CustomFileDialog::Display("SaveProfilerRawKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string filePath = Frontend::CustomFileDialog::GetFilePathName();

            std::ofstream outFile(filePath);
            if (outFile.is_open()) {
                outFile << GenerateProfilerRawData(readCounts, writeCounts);
                outFile.close();
            }
        }
        Frontend::CustomFileDialog::Close();
    }
}
#endif

}  // namespace

void DrawProfilerWindow(AppState& state) {
    std::span<const uint32_t> counts(state.emulator.GetProfilerCounts(), 65536);
    std::span<const uint32_t> readCounts(state.emulator.GetProfilerReadCounts(), 65536);
    std::span<const uint32_t> writeCounts(state.emulator.GetProfilerWriteCounts(), 65536);
    static std::array<unsigned char, 256ULL * 256ULL * 3ULL> pixels{};
    static bool profilerDirty = true;

    if ((!state.emulator.IsPaused() && !state.emulator.IsHalted()) || profilerDirty) {
        UpdateProfilerTexture(state, readCounts, writeCounts, pixels);
        profilerDirty = false;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float size = std::min(avail.x, avail.y) * 0.9F;

    ImGui::SetCursorPos(ImVec2((avail.x - size) * 0.5F, (avail.y - size) * 0.5F));
    ImGui::Image((ImTextureID)(intptr_t)state.render.profilerTexture, ImVec2(size, size));

    if (ImGui::IsItemHovered()) {
        DrawProfilerTooltip(counts, readCounts, writeCounts, size, size);
    }

    DrawProfilerButtons(state, readCounts, writeCounts, pixels, profilerDirty);
#ifndef TARGET_WASM
    HandleProfilerDialogs(readCounts, writeCounts, pixels);
#endif
}

}  // namespace GUI
