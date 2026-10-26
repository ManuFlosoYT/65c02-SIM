#include "Frontend/GUI/Video/VRAMImageIO.h"

#include <glad/gl.h>
#include <SDL3/SDL.h>

#include <array>
#include <bit>
#include <ctime>
#include <fstream>
#include <vector>

#include "Frontend/GUI/CustomFileDialog.h"

#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif

namespace GUI {

void VRAMImageIO::ConvertVRAMToRGB(const Hardware::GPU& gpu, uint8_t* dst) {
    for (size_t yIndex = 0; yIndex < Hardware::GPU::VRAM_HEIGHT; yIndex++) {
        for (size_t xIndex = 0; xIndex < Hardware::GPU::VRAM_WIDTH; xIndex++) {
            size_t idx = (yIndex * Hardware::GPU::VRAM_WIDTH + xIndex) * 3;
            Core::Byte val = gpu.GetVRAM().at(yIndex).at(xIndex);
            dst[idx + 0] = ((val >> 4) & 0x03) * 85;
            dst[idx + 1] = ((val >> 2) & 0x03) * 85;
            dst[idx + 2] = (val & 0x03) * 85;
        }
    }
}

void VRAMImageIO::LoadVRAMFromFile(const std::string& imgPath, Control::AppState& state) {
    std::ifstream file(imgPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize > 0) {
        file.seekg(0, std::ios::beg);
        std::vector<unsigned char> buf(static_cast<size_t>(fileSize));
        if (file.read(std::bit_cast<char*>(buf.data()), fileSize)) {
            Hardware::GPU& gpu = state.emulator.GetGPU();
            gpu.LoadVRAM(buf);
        }
    }
}

void VRAMImageIO::ExportVRAMBMP(Control::AppState& state) {
#ifdef TARGET_WASM
    std::string tempPath = "/tmp/vram_capture.bmp";
    std::vector<unsigned char> pixels;
    int capW = Hardware::GPU::VRAM_WIDTH;
    int capH = Hardware::GPU::VRAM_HEIGHT;
    pixels.resize(static_cast<std::size_t>(capW) * static_cast<std::size_t>(capH) * 3);
    Hardware::GPU& gpu = state.emulator.GetGPU();
    ConvertVRAMToRGB(gpu, pixels.data());
    SDL_Surface* surface = SDL_CreateSurfaceFrom(capW, capH, SDL_PIXELFORMAT_RGB24, pixels.data(), capW * 3);
    if (surface != nullptr) {
        SDL_SaveBMP(surface, tempPath.c_str());
        SDL_DestroySurface(surface);
        std::ifstream ifs(tempPath, std::ios::binary);
        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        WebFileUtils::download_file("vram_capture.bmp", buffer.data(), buffer.size());
    }
#else
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
#endif
}

#ifndef TARGET_WASM
void VRAMImageIO::HandleVRAMLoadDialog(Control::AppState& state) {
    if (!Frontend::CustomFileDialog::Display("ChooseVRAMImageKey")) {
        return;
    }

    if (Frontend::CustomFileDialog::IsOk()) {
        std::string imgPath = Frontend::CustomFileDialog::GetFilePathName();
        LoadVRAMFromFile(imgPath, state);
    }
    Frontend::CustomFileDialog::Close();
}

void VRAMImageIO::HandleVRAMSaveDialog(Control::AppState& state) {
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
            capW = Hardware::GPU::VRAM_WIDTH;
            capH = Hardware::GPU::VRAM_HEIGHT;
            pixels.resize(static_cast<std::size_t>(capW) * static_cast<std::size_t>(capH) * 3);
            Hardware::GPU& gpu = state.emulator.GetGPU();
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

}  // namespace GUI
