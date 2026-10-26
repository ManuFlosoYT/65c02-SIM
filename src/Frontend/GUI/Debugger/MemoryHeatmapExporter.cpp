#include "Frontend/GUI/Debugger/MemoryHeatmapExporter.h"

#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "Frontend/GUI/CustomFileDialog.h"

#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif

namespace GUI {

void MemoryHeatmapExporter::UpdateTexture(Control::AppState& state,
                                         std::span<const uint32_t> readCounts,
                                         std::span<const uint32_t> writeCounts,
                                         std::span<unsigned char> pixels) {
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
            pixels[idx + 0] = static_cast<unsigned char>(rIntensity * 255.0F);
            pixels[idx + 1] = 0;
            pixels[idx + 2] = static_cast<unsigned char>(bIntensity * 255.0F);
        }
    } else {
        std::fill(pixels.begin(), pixels.end(), 0);
    }

    if (state.render.profilerTexture == 0) {
        glGenTextures(1, &state.render.profilerTexture);
        glBindTexture(GL_TEXTURE_2D, state.render.profilerTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, state.render.profilerTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::string MemoryHeatmapExporter::GenerateRawData(std::span<const uint32_t> readCounts,
                                                   std::span<const uint32_t> writeCounts) {
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

void MemoryHeatmapExporter::DrawControls(Control::AppState& state,
                                        std::span<const uint32_t> readCounts,
                                        std::span<const uint32_t> writeCounts,
                                        std::span<unsigned char> pixels,
                                        bool& profilerDirty) {
    if (ImGui::Button("Clear Data")) {
        state.emulator.ClearProfiler();
        state.emulator.GetSubroutineProfiler().Clear();
        profilerDirty = true;
    }
    ImGui::SameLine();
#ifdef TARGET_WASM
    if (ImGui::Button("Export RAW")) {
        std::string raw = GenerateRawData(readCounts, writeCounts);
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
void MemoryHeatmapExporter::HandleDialogs(Control::AppState& state,
                                         std::span<const uint32_t> readCounts,
                                         std::span<const uint32_t> writeCounts,
                                         std::span<unsigned char> pixels) {
    if (Frontend::CustomFileDialog::Display("ProfilerLoadDbgKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string filePath = Frontend::CustomFileDialog::GetFilePathName();
            if (state.rom.symbols.LoadFromFile(filePath)) {
                state.emulator.GetSubroutineProfiler().SetSymbolTable(&state.rom.symbols);
            }
        }
        Frontend::CustomFileDialog::Close();
    }

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

    if (Frontend::CustomFileDialog::Display("SaveProfilerRawKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string filePath = Frontend::CustomFileDialog::GetFilePathName();

            std::ofstream outFile(filePath);
            if (outFile.is_open()) {
                outFile << GenerateRawData(readCounts, writeCounts);
                outFile.close();
            }
        }
        Frontend::CustomFileDialog::Close();
    }
}
#endif

}  // namespace GUI
