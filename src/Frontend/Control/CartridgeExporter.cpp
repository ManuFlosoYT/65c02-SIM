#include "Frontend/Control/CartridgeExporter.h"
#include <miniz.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <format>
#include <SDL3/SDL.h>
#include <imgui.h>

namespace Control {

std::string CartridgeExporter::Base64UrlEncode(const uint8_t* data, size_t length) {
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string encoded;
    encoded.reserve(((length + 2) / 3) * 4);
    for (size_t i = 0; i < length; i += 3) {
        uint32_t val = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < length) {
            val |= static_cast<uint32_t>(data[i + 1]) << 8;
        }
        if (i + 2 < length) {
            val |= static_cast<uint32_t>(data[i + 2]);
        }
        encoded.push_back(table[(val >> 18) & 0x3F]);
        encoded.push_back(table[(val >> 12) & 0x3F]);
        if (i + 1 < length) {
            encoded.push_back(table[(val >> 6) & 0x3F]);
        }
        if (i + 2 < length) {
            encoded.push_back(table[val & 0x3F]);
        }
    }
    return encoded;
}

nlohmann::json CartridgeExporter::BuildBusJson(const AppState& state) {
    auto busArray = nlohmann::json::array();
    for (const auto& reg : state.emulator.GetMem().GetRegisteredDevices()) {
        if (reg.isVirtual || !reg.device) {
            continue;
        }
        busArray.push_back(nlohmann::json::object({
            {"name", reg.device->GetName()},
            {"start", std::format("0x{:04X}", reg.startAddress)},
            {"end", std::format("0x{:04X}", reg.endAddress)}
        }));
    }
    return busArray;
}

nlohmann::json CartridgeExporter::GenerateManifest(const AppState& state,
                                                   const std::string& name,
                                                   const std::string& author,
                                                   const std::string& description,
                                                   const std::string& version,
                                                   int sidModel) {
    nlohmann::json manifest;
    manifest["version"] = "2.2";
    manifest["metadata"] = {
        {"name", name},
        {"author", author},
        {"description", description},
        {"version", version},
        {"type", "rom"}
    };
    manifest["rom"] = "rom.bin";
    const auto& cart = state.emulator.GetCartridge();
    if (cart.loaded && !cart.vramData.empty()) {
        manifest["vram"] = "vram.bin";
    }
    manifest["config"] = {
        {"target_ips", state.emulation.instructionsPerFrame},
        {"gpu_enabled", state.emulation.gpuEnabled},
        {"cycle_accurate", state.emulation.cycleAccurate},
        {"sid_enabled", state.emulator.GetSID().IsSoundEnabled()},
        {"sid_model", sidModel == 0 ? "6581" : "8580"},
        {"esp_enabled", state.emulation.espEnabled},
        {"sd_enabled", state.emulation.sdEnabled}
    };
    manifest["bus"] = BuildBusJson(state);
    return manifest;
}

bool CartridgeExporter::ExportCartridgeToFile(const AppState& state,
                                              const std::string& manifestStr,
                                              const std::string& outPath) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (mz_zip_writer_init_file(&zip, outPath.c_str(), 0) == MZ_FALSE) {
        return false;
    }

    mz_zip_writer_add_mem(&zip, "manifest.json", manifestStr.data(), manifestStr.size(), MZ_BEST_COMPRESSION);

    const auto& cart = state.emulator.GetCartridge();
    if (cart.loaded && !cart.romData.empty()) {
        mz_zip_writer_add_mem(&zip, "rom.bin", cart.romData.data(), cart.romData.size(), MZ_BEST_COMPRESSION);
    } else if (!state.rom.data.empty()) {
        mz_zip_writer_add_mem(&zip, "rom.bin", state.rom.data.data(), state.rom.data.size(), MZ_BEST_COMPRESSION);
    } else if (!state.rom.bin.empty() && std::filesystem::exists(state.rom.bin)) {
        mz_zip_writer_add_file(&zip, "rom.bin", state.rom.bin.c_str(), nullptr, 0, MZ_BEST_COMPRESSION);
    }

    if (cart.loaded && !cart.vramData.empty()) {
        mz_zip_writer_add_mem(&zip, "vram.bin", cart.vramData.data(), cart.vramData.size(), MZ_BEST_COMPRESSION);
    }

    if (state.emulator.GetSDCard().IsMounted()) {
        std::string sdPath = state.emulator.GetSDCard().GetMountedPath();
        if (!mz_zip_writer_add_file(&zip, "sdcard.img", sdPath.c_str(), nullptr, 0, MZ_BEST_COMPRESSION)) {
            std::cerr << "Warning: Could not read mounted SD Card file for export: " << sdPath << "\n";
        }
    }

    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);
    return true;
}

bool CartridgeExporter::ExportCartridgeToUrl(AppState& state, const std::string& manifestStr) {
    std::string tempPath = (std::filesystem::temp_directory_path() / "temp_export.65c").string();
    if (!ExportCartridgeToFile(state, manifestStr, tempPath)) {
        return false;
    }

    std::ifstream f(tempPath, std::ios::binary);
    if (!f.is_open()) {
        return false;
    }
    std::vector<uint8_t> zipBytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    std::error_code ec;
    std::filesystem::remove(tempPath, ec);

    if (zipBytes.empty()) {
        return false;
    }

    auto origSize = static_cast<uint32_t>(zipBytes.size());
    mz_ulong bound = mz_compressBound(origSize);
    std::vector<uint8_t> compressed(4 + bound);
    compressed[0] = static_cast<uint8_t>(origSize & 0xFF);
    compressed[1] = static_cast<uint8_t>((origSize >> 8) & 0xFF);
    compressed[2] = static_cast<uint8_t>((origSize >> 16) & 0xFF);
    compressed[3] = static_cast<uint8_t>((origSize >> 24) & 0xFF);

    mz_ulong compSize = bound;
    if (mz_compress(compressed.data() + 4, &compSize, zipBytes.data(), origSize) != MZ_OK) {
        return false;
    }
    compressed.resize(4 + compSize);

    std::string b64 = Base64UrlEncode(compressed.data(), compressed.size());
    std::string fullUrl = "https://manufloso.dev/65c02-SIM/#code=" + b64;

    SDL_SetClipboardText(fullUrl.c_str());
    ImGui::SetClipboardText(fullUrl.c_str());
    SDL_OpenURL(fullUrl.c_str());
    std::cout << "URL copied to clipboard: " << fullUrl << '\n';

    return true;
}

} // namespace Control
