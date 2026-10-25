#include "Frontend/GUI/Debugger/SettingsWindow.h"

#include "Frontend/GUI/CustomFileDialog.h"
#include <SDL3/SDL.h>
#include <imgui.h>

#include <array>
#include <chrono>
#include <ctime>
#include <format>
#include <iterator>
#include <sstream>
#include <vector>
#include <filesystem>
#include <miniz.h>
#include <nlohmann/json.hpp>
#include <iostream>

#include "Frontend/Control/SDUtils.h"
#ifdef TARGET_WASM
#include <fstream>
#include "Frontend/web/WebFileUtils.h"
#endif

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

// Implemented in SDUtils.cpp

static void DrawEmulationSettings(AppState& state) {
    ImGui::SeparatorText("Emulation");
    bool cycleOverridden = state.emulator.GetCartridge().config.cycleAccurate.has_value();
    ImGui::BeginDisabled(cycleOverridden);
    if (ImGui::Checkbox("Cycle-Accurate", &state.emulation.cycleAccurate)) {
        state.emulator.SetCycleAccurate(state.emulation.cycleAccurate);
    }
    ImGui::EndDisabled();
    if (cycleOverridden && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Managed by Cartridge");
            ImGui::EndTooltip();
        }
    }

    ImGui::Checkbox("Force load savestate", &state.emulation.forceLoadSaveState);
#ifdef TARGET_WASM
    bool dummyAutoReload = false;
    ImGui::BeginDisabled(true);
    ImGui::Checkbox("Auto-Reload Bin", &dummyAutoReload);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Auto-reload is not supported in the web build.");
            ImGui::EndTooltip();
        }
    }
#else
    if (ImGui::Checkbox("Auto-Reload Bin", &state.emulation.autoReload)) {
        state.emulator.SetAutoReload(state.emulation.autoReload);
    }
#endif
    if (ImGui::Checkbox("Simulate SD Read Latency", &state.emulation.sdLatencyEnabled)) {
        state.emulator.GetSDCard().SetReadLatencyEnabled(state.emulation.sdLatencyEnabled);
    }
    if (ImGui::Checkbox("Random RAM at Power-On", &state.emulation.ramPowerOnPattern)) {
        state.emulator.GetRAM().SetPowerOnPattern(state.emulation.ramPowerOnPattern);
    }
    ImGui::SetItemTooltip("Fills RAM with random bytes on next Reset to catch uninitialized variables.");
}

static void DrawScriptingSettings(AppState& state) {
    ImGui::SeparatorText("Scripting");

    if (state.script.loaded) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0F, 1.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("Script Loaded & Running");
        ImGui::PopStyleColor();
        if (ImGui::Button("Stop Script", ImVec2(-FLT_MIN, 0))) {
            state.emulator.GetScriptEngine().Stop();
            state.script.loaded = false;
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("No Script Loaded");
        ImGui::PopStyleColor();
#ifdef TARGET_WASM
        ImGui::BeginDisabled();
        if (ImGui::Button("Load & Run Script (.s65)", ImVec2(-FLT_MIN, 0))) {
            WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
                std::string virtualPath = "/tmp/" + std::string(filename);
                FILE* f = fopen(virtualPath.c_str(), "wb");
                if (f) {
                    fwrite(data, 1, size, f);
                    fclose(f);
                }

                state.script.path = virtualPath;
                state.script.loaded = true;
                state.script.showConsole = true;
                state.emulator.Pause();
                state.emulator.GetScriptEngine().LoadAndRun(virtualPath);
            };
            WebFileUtils::open_browser_file_picker(".s65");
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (ImGui::BeginItemTooltip()) {
                ImGui::TextUnformatted("S65 scripting is not supported in the web build yet.");
                ImGui::EndTooltip();
            }
        }
#else
        if (ImGui::Button("Load & Run Script (.s65)", ImVec2(-FLT_MIN, 0))) {
            Frontend::CustomFileDialog::OpenDialog("LoadScriptDlgKey", "Load S65 Script", ".s65", ".", "", 1, nullptr,
                                                    ImGuiFileDialogFlags_None);
        }
#endif
    }
}

static void DrawSDCardSettings(AppState& state) {
    ImGui::SeparatorText("Storage (SD Card)");

    bool cartridgeHasSD = !state.emulator.GetCartridge().sdCardPath.empty();
    bool sdMounted = state.emulator.GetSDCard().IsMounted();
    if (sdMounted) {
        if (state.emulator.GetSDCard().IsFolderMount()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0F, 1.0F, 0.0F, 1.0F));
            ImGui::TextUnformatted("SD Host Folder Mounted");
            ImGui::PopStyleColor();
            ImGui::Text("Path: %s", state.emulator.GetSDCard().GetMountedPath().c_str());
            if (state.emulator.GetSDCard().IsDeletionAllowed()) {
                ImGui::TextUnformatted("Host Deletions: Allowed");
            } else {
                ImGui::TextUnformatted("Host Deletions: Restricted");
            }
            if (ImGui::Button("Sync Folder", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0))) {
                state.emulator.GetSDCard().SyncFromHost();
            }
            ImGui::SameLine();
            if (ImGui::Button("Unmount SD Card", ImVec2(-FLT_MIN, 0))) {
                state.emulator.GetSDCard().Unmount();
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0F, 1.0F, 0.0F, 1.0F));
            ImGui::TextUnformatted("SD Card Mounted");
            ImGui::PopStyleColor();
#ifdef TARGET_WASM
            if (ImGui::Button("Save Changes", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0))) {
                std::string path = state.emulator.GetSDCard().GetMountedPath();
                if (!path.empty()) {
                    std::ifstream ifs(path, std::ios::binary);
                    if (ifs.is_open()) {
                        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                        std::string filename = std::filesystem::path(path).filename().string();
                        WebFileUtils::download_file(filename.c_str(), buffer.data(), buffer.size());
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Unmount", ImVec2(-FLT_MIN, 0))) {
                state.emulator.GetSDCard().Unmount();
            }
#else
            if (ImGui::Button("Unmount SD Card", ImVec2(-FLT_MIN, 0))) {
                state.emulator.GetSDCard().Unmount();
            }
#endif
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("SD Card Not Mounted");
        ImGui::PopStyleColor();
        ImGui::BeginDisabled(cartridgeHasSD);
#ifdef TARGET_WASM
        if (ImGui::Button("Create New", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0))) {
            std::string tempPath = "/tmp/new_sd.img";
            if (GUI::CreateFAT16Image(tempPath)) {
                std::ifstream ifs(tempPath, std::ios::binary);
                std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                WebFileUtils::download_file("new_sd.img", buffer.data(), buffer.size());
                
                if (GUI::IsSDCardEnabled(state)) {
                    state.emulator.GetSDCard().Mount(tempPath);
                    state.popups.sdCardWebWarning = true;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Mount IMG", ImVec2(-FLT_MIN, 0))) {
            WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
                std::string virtualPath = "/tmp/" + std::string(filename);
                FILE* f = fopen(virtualPath.c_str(), "wb");
                if (f) {
                    fwrite(data, 1, size, f);
                    fclose(f);
                }

                if (GUI::IsSDCardEnabled(state)) {
                    state.emulator.GetSDCard().Mount(virtualPath);
                    state.popups.sdCardWebWarning = true;
                } else {
                    state.popups.sdCardDisabled = true;
                }
            };
            WebFileUtils::open_browser_file_picker(".img");
        }
#else
        if (ImGui::Button("Create New", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0))) {
            Frontend::CustomFileDialog::OpenDialog("CreateSDDlgKey", "Save New SD Image", ".img", ".", "", 1, nullptr,
                                                    ImGuiFileDialogFlags_ConfirmOverwrite);
        }
        ImGui::SameLine();
        if (ImGui::Button("Mount IMG", ImVec2(-FLT_MIN, 0))) {
            Frontend::CustomFileDialog::OpenDialog("MountSDDlgKey", "Mount SD Image", ".img", ".", "", 1, nullptr,
                                                    ImGuiFileDialogFlags_None);
        }
        if (ImGui::Button("Mount Folder", ImVec2(-FLT_MIN, 0))) {
            Frontend::CustomFileDialog::OpenDialog("MountFolderDlgKey", "Mount Host Folder", nullptr, ".");
        }
#endif
        ImGui::EndDisabled();
        if (cartridgeHasSD && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (ImGui::BeginItemTooltip()) {
                ImGui::TextUnformatted("SD Card is managed by the active cartridge.");
                ImGui::EndTooltip();
            }
        }
    }
}

static void DrawSettingsSaveState(AppState& state) {
    ImGui::SeparatorText("Save States");

#ifdef TARGET_WASM
    if (ImGui::Button("Save State", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0))) {
        std::string tempPath = "/tmp/save.savestate";
        if (state.emulator.SaveState(tempPath)) {
            std::ifstream ifs(tempPath, std::ios::binary);
            std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            WebFileUtils::download_file("emulator_state.savestate", buffer.data(), buffer.size());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load State", ImVec2(-FLT_MIN, 0))) {
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            std::string virtualPath = "/tmp/" + std::string(filename);
            FILE* f = fopen(virtualPath.c_str(), "wb");
            if (f) {
                fwrite(data, 1, size, f);
                fclose(f);
            }

            state.emulator.Pause();
            state.emulator.LoadState(virtualPath, state.emulation.forceLoadSaveState);
            state.rom.loaded = true;
            state.emulation.gpuEnabled = state.emulator.IsGPUEnabled();
            state.emulation.instructionsPerFrame = state.emulator.GetTargetIPS();
            state.emulation.cycleAccurate = state.emulator.IsCycleAccurate();
            state.emulation.autoReload = state.emulator.IsAutoReloadEnabled();
            state.emulation.sdLatencyEnabled = state.emulator.GetSDCard().IsReadLatencyEnabled();
            state.rom.bin = state.emulator.GetCurrentBinPath();
        };
        WebFileUtils::open_browser_file_picker(".savestate");
    }
#else
    if (ImGui::Button("Save State", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0))) {
        std::string binName = "unknown";
        if (!state.rom.bin.empty()) {
            binName = std::filesystem::path(state.rom.bin).stem().string();
        }
        auto now = std::chrono::system_clock::now();
        std::time_t timeValue = std::chrono::system_clock::to_time_t(now);
        std::tm* timeStruct = std::localtime(&timeValue);
        std::ostringstream dateStr;
        dateStr << (timeStruct->tm_year + 1900) << (timeStruct->tm_mon + 1 < 10 ? "0" : "") << (timeStruct->tm_mon + 1)
                << (timeStruct->tm_mday < 10 ? "0" : "") << timeStruct->tm_mday << "_"
                << (timeStruct->tm_hour < 10 ? "0" : "") << timeStruct->tm_hour << (timeStruct->tm_min < 10 ? "0" : "")
                << timeStruct->tm_min << (timeStruct->tm_sec < 10 ? "0" : "") << timeStruct->tm_sec;
        std::string dateOutput = dateStr.str();
        std::string defaultName = "SIM65C02SST_" + binName + "_" + dateOutput;
        Frontend::CustomFileDialog::OpenDialog("SaveStateDlgKey", "Save State", ".savestate", ".", defaultName);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load State", ImVec2(-FLT_MIN, 0))) {
        Frontend::CustomFileDialog::OpenDialog("LoadStateDlgKey", "Load State", ".savestate", ".");
    }
#endif
}

static void DrawSettingsCRTPart1(AppState& state) {
    ImGui::SeparatorText("Visuals (CRT Part 1)");
    auto setCRTAll = [&](bool val) {
        state.crt.scanlines = state.crt.interlacing = state.crt.curvature = state.crt.chromatic = state.crt.blur =
            state.crt.shadowMask = state.crt.vignette = state.crt.cornerRounding = state.crt.glassGlare =
                state.crt.colorBleeding = state.crt.noise = state.crt.vsyncJitter = state.crt.phosphorDecay =
                    state.crt.bloom = state.crt.ghosting = state.crt.halation = state.crt.moire =
                        state.crt.sharpPixels = state.crt.humBar = val;
        state.crt.monitorType = CRTMonitorType::RGB;
        state.crt.gamma = val ? 2.75F : 2.2F;
    };
    if (ImGui::Button("All On", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0))) {
        setCRTAll(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("All Off", ImVec2(-FLT_MIN, 0))) {
        setCRTAll(false);
    }
    ImGui::TextUnformatted("Essentials");
    ImGui::Checkbox("Scanlines", &state.crt.scanlines);
    ImGui::Checkbox("Interlacing", &state.crt.interlacing);
    ImGui::Checkbox("Screen Curvature", &state.crt.curvature);
    ImGui::Checkbox("Chromatic Aberration", &state.crt.chromatic);
    ImGui::Checkbox("Phosphor Blur", &state.crt.blur);
    ImGui::Spacing();
    ImGui::TextUnformatted("Screen Physicality");
    ImGui::Checkbox("Shadow Mask", &state.crt.shadowMask);
    ImGui::Checkbox("Vignette", &state.crt.vignette);
    ImGui::Checkbox("Corner Rounding", &state.crt.cornerRounding);
    ImGui::Checkbox("Glass Glare", &state.crt.glassGlare);
}

static void DrawSettingsCRTPart2(AppState& state) {
    ImGui::SeparatorText("Visuals (CRT Part 2)");
    ImGui::TextUnformatted("Signal & Analog");
    ImGui::Checkbox("Color Bleeding", &state.crt.colorBleeding);
    ImGui::Checkbox("RF Noise", &state.crt.noise);
    ImGui::Checkbox("VSync Jitter", &state.crt.vsyncJitter);
    ImGui::Checkbox("Phosphor Decay", &state.crt.phosphorDecay);
    ImGui::Spacing();
    ImGui::TextUnformatted("Lighting");
    ImGui::Checkbox("Bloom", &state.crt.bloom);
    ImGui::Checkbox("Halation", &state.crt.halation);
    ImGui::Spacing();
    ImGui::TextUnformatted("Signal Advanced");
    ImGui::Checkbox("Ghosting (Echo)", &state.crt.ghosting);
    ImGui::Checkbox("Moiré Pattern", &state.crt.moire);
    ImGui::SliderFloat("Gamma", &state.crt.gamma, 1.0F, 3.5F, "%.2f");
    ImGui::Spacing();
    ImGui::TextUnformatted("Display Config");
    ImGui::Checkbox("Sharp Pixels", &state.crt.sharpPixels);
    ImGui::Checkbox("Hum Bar", &state.crt.humBar);
    static constexpr std::array<const char*, 4> monitorTypes = {"RGB", "Green P1", "Amber P3", "White P4"};
    int monitorIdx = static_cast<int>(state.crt.monitorType);
    if (ImGui::Combo("Monitor", &monitorIdx, monitorTypes.data(), static_cast<int>(monitorTypes.size()))) {
        state.crt.monitorType = static_cast<CRTMonitorType>(monitorIdx);
    }
    ImGui::Spacing();
    ImGui::TextUnformatted("Electron Beam");
    if (ImGui::Checkbox("Raster Beam Visualization", &state.crt.rasterBeam)) {
        state.emulator.SetRasterBeamSlowMotion(state.crt.rasterBeam);
    }
}

static char nameBuf[64] = "My Cartridge";
static char authorBuf[64] = "Unknown";
static char descBuf[128] = "";
static char verBuf[16] = "1.1";
static int exportSidModel = 1; // 0=6581, 1=8580

static std::string pendingManifestStr;

static std::string Base64UrlEncode(const uint8_t* data, size_t length) {
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

static bool ExportCartridgeToUrl(AppState& state, const std::string& manifestStr) {
    std::string tempPath = (std::filesystem::temp_directory_path() / "temp_export.65c").string();
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (mz_zip_writer_init_file(&zip, tempPath.c_str(), 0) == MZ_FALSE) {
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

static nlohmann::json BuildBusJson(const AppState& state) {
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

static void DrawExportCartridgePopup(AppState& state) {
    if (ImGui::BeginPopupModal("Export ROM to Cartridge###ExportCartridgePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name", nameBuf, sizeof(nameBuf));
        ImGui::InputText("Author", authorBuf, sizeof(authorBuf));
        ImGui::InputText("Description", descBuf, sizeof(descBuf));
        ImGui::InputText("Version", verBuf, sizeof(verBuf));

        ImGui::SeparatorText("Hardware Configuration (Copied)");
        ImGui::Text("Target IPS: %d", state.emulation.instructionsPerFrame);
        ImGui::Text("GPU: %s", state.emulation.gpuEnabled ? "Enabled" : "Disabled");
        ImGui::Text("Cycle Accurate: %s", state.emulation.cycleAccurate ? "Enabled" : "Disabled");
        ImGui::Text("SID: %s", state.emulator.GetSID().IsSoundEnabled() ? "Enabled" : "Disabled");
        
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("SID Model:");
        ImGui::SameLine();
        ImGui::PushItemWidth(100);
        const char* sidModels[] = { "MOS 6581", "MOS 8580" };
        ImGui::Combo("##ExportSIDModel", &exportSidModel, sidModels, 2);
        ImGui::PopItemWidth();
        
        ImGui::Text("ESP: %s", state.emulation.espEnabled ? "Enabled" : "Disabled");
        ImGui::Text("SD Card: %s", state.emulation.sdEnabled ? "Enabled" : "Disabled");

        ImGui::Separator();
        if (ImGui::Button("Export", ImVec2(120, 0))) {
            nlohmann::json manifest;
            manifest["version"] = "2.2";
            manifest["metadata"] = {
                {"name", nameBuf},
                {"author", authorBuf},
                {"description", descBuf},
                {"version", verBuf},
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
                {"sid_model", exportSidModel == 0 ? "6581" : "8580"},
                {"esp_enabled", state.emulation.espEnabled},
                {"sd_enabled", state.emulation.sdEnabled}
            };
            manifest["bus"] = BuildBusJson(state);

            pendingManifestStr = manifest.dump(2);
            
            std::filesystem::path binPath(state.rom.bin);
            std::string cartName = binPath.stem().string() + ".65c";
            
            Frontend::CustomFileDialog::OpenDialog("ExportCartridgeDlgKey", "Export Cartridge", ".65c", ".", cartName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Export to URL", ImVec2(120, 0))) {
            nlohmann::json manifest;
            manifest["version"] = "2.2";
            manifest["metadata"] = {
                {"name", nameBuf},
                {"author", authorBuf},
                {"description", descBuf},
                {"version", verBuf},
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
                {"sid_model", exportSidModel == 0 ? "6581" : "8580"},
                {"esp_enabled", state.emulation.espEnabled},
                {"sd_enabled", state.emulation.sdEnabled}
            };
            manifest["bus"] = BuildBusJson(state);
            ExportCartridgeToUrl(state, manifest.dump(2));
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawSettingsContent(AppState& state) {
    bool openExportCartridgePopup = false;
    ImGui::BeginChild("SettingsContent", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    if (ImGui::BeginTable("SettingsLayoutTable", 3, ImGuiTableFlags_None)) {
        ImGui::TableNextColumn();
        DrawEmulationSettings(state);
        ImGui::Spacing();
        DrawScriptingSettings(state);
        ImGui::Spacing();
        DrawSDCardSettings(state);
        ImGui::Spacing();
        DrawSettingsSaveState(state);
        ImGui::Spacing();

        if (state.rom.loaded) {
            ImGui::SeparatorText("Export");
#ifdef TARGET_WASM
            ImGui::BeginDisabled(true);
            ImGui::Button("Export to Cartridge");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                if (ImGui::BeginItemTooltip()) {
                    ImGui::TextUnformatted("Export to cartridge is not supported in the web version");
                    ImGui::EndTooltip();
                }
            }
            ImGui::EndDisabled();
#else
            if (ImGui::Button("Export to Cartridge")) {
                openExportCartridgePopup = true;
            }
#endif
            if (ImGui::Button("Export to URL")) {
                std::filesystem::path binPath(state.rom.bin);
                std::string romTitle = binPath.stem().empty() ? "My Cartridge" : binPath.stem().string();
                const auto& cart = state.emulator.GetCartridge();
                nlohmann::json manifest;
                manifest["version"] = "2.2";
                if (cart.loaded) {
                    manifest["metadata"] = {
                        {"name", cart.metadata.name.empty() ? romTitle : cart.metadata.name},
                        {"author", cart.metadata.author.empty() ? "Unknown" : cart.metadata.author},
                        {"description", cart.metadata.description},
                        {"version", cart.metadata.version.empty() ? "1.1" : cart.metadata.version},
                        {"type", "rom"}
                    };
                } else {
                    manifest["metadata"] = {
                        {"name", romTitle},
                        {"author", authorBuf},
                        {"description", descBuf},
                        {"version", verBuf},
                        {"type", "rom"}
                    };
                }
                manifest["rom"] = "rom.bin";
                if (cart.loaded && !cart.vramData.empty()) {
                    manifest["vram"] = "vram.bin";
                }
                manifest["config"] = {
                    {"target_ips", state.emulation.instructionsPerFrame},
                    {"gpu_enabled", state.emulation.gpuEnabled},
                    {"cycle_accurate", state.emulation.cycleAccurate},
                    {"sid_enabled", state.emulator.GetSID().IsSoundEnabled()},
                    {"sid_model", exportSidModel == 0 ? "6581" : "8580"},
                    {"esp_enabled", state.emulation.espEnabled},
                    {"sd_enabled", state.emulation.sdEnabled}
                };
                manifest["bus"] = BuildBusJson(state);
                ExportCartridgeToUrl(state, manifest.dump(2));
            }
        }

        ImGui::TableNextColumn();
        DrawSettingsCRTPart1(state);

        ImGui::TableNextColumn();
        DrawSettingsCRTPart2(state);

        ImGui::EndTable();
    }
    
    if (openExportCartridgePopup) {
        ImGui::OpenPopup("ExportCartridgePopup");
    }
    DrawExportCartridgePopup(state);

    if (Frontend::CustomFileDialog::Display("ExportCartridgeDlgKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string outPath = Frontend::CustomFileDialog::GetFilePathName();
            mz_zip_archive zip_archive;
            memset(&zip_archive, 0, sizeof(zip_archive));
            if (mz_zip_writer_init_file(&zip_archive, outPath.c_str(), 0)) {
                mz_zip_writer_add_mem(&zip_archive, "manifest.json", pendingManifestStr.c_str(), pendingManifestStr.size(), MZ_BEST_COMPRESSION);
                const auto& cart = state.emulator.GetCartridge();
                if (cart.loaded && !cart.romData.empty()) {
                    mz_zip_writer_add_mem(&zip_archive, "rom.bin", cart.romData.data(), cart.romData.size(), MZ_BEST_COMPRESSION);
                } else if (!state.rom.data.empty()) {
                    mz_zip_writer_add_mem(&zip_archive, "rom.bin", state.rom.data.data(), state.rom.data.size(), MZ_BEST_COMPRESSION);
                } else if (!state.rom.bin.empty()) {
                    mz_zip_writer_add_file(&zip_archive, "rom.bin", state.rom.bin.c_str(), nullptr, 0, MZ_BEST_COMPRESSION);
                }
                if (cart.loaded && !cart.vramData.empty()) {
                    mz_zip_writer_add_mem(&zip_archive, "vram.bin", cart.vramData.data(), cart.vramData.size(), MZ_BEST_COMPRESSION);
                }
                
                if (state.emulator.GetSDCard().IsMounted()) {
                    std::string sdPath = state.emulator.GetSDCard().GetMountedPath();
                    if (!mz_zip_writer_add_file(&zip_archive, "sdcard.img", sdPath.c_str(), nullptr, 0, MZ_BEST_COMPRESSION)) {
                        std::cerr << "Warning: Could not read mounted SD Card file for export: " << sdPath << "\n";
                    }
                }

                mz_zip_writer_finalize_archive(&zip_archive);
                mz_zip_writer_end(&zip_archive);
                std::cout << "Cartridge exported successfully to " << outPath << "\n";
            } else {
                std::cerr << "Failed to initialize zip for cartridge export: " << outPath << "\n";
            }
        }
        Frontend::CustomFileDialog::Close();
    }

    ImGui::EndChild();
}

}  // namespace GUI
