#include "Frontend/Control/SDKManager.h"
#include "Frontend/Control/AppState.h"

#include <miniz.h>
#include <array>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_set>
#include <algorithm>

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerVFS.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#if !defined(TARGET_WASM)
#include "sdk_zip.h"
#endif

namespace fs = std::filesystem;

namespace Control {

static fs::path GetSDKDir() {
#if defined(_WIN32)
    std::array<char, MAX_PATH> buffer{};
    if (GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size())) > 0) {
        return fs::path(buffer.data()).parent_path() / "SDK";
    }
#elif !defined(TARGET_WASM)
    std::error_code ec;
    auto exePath = fs::read_symlink("/proc/self/exe", ec);
    if (!ec && !exePath.empty()) {
        return exePath.parent_path() / "SDK";
    }
#endif
    return fs::current_path() / "SDK";
}

void SDKManager::CheckSDKStatus(AppState& state) {
#if defined(TARGET_WASM)
    return;
#else
    const fs::path sdkDir = GetSDKDir();
    if (!fs::exists(sdkDir)) {
        std::cout << "SDKManager: SDK folder missing. Performing initial extraction..." << "\n";
        ExtractBundledSDK();
        ScanExtractedSDK(state);
        return;
    }

    std::string extractedVersion = GetExtractedVersion();
    std::string currentVersion = std::string(PROJECT_VERSION);

    if (extractedVersion.empty() || extractedVersion != currentVersion) {
        std::cout << "SDKManager: SDK version mismatch (Extracted: " << (extractedVersion.empty() ? "None" : extractedVersion)
                  << ", Current: " << currentVersion << "). Queuing update popup..." << "\n";
        state.sdk.showUpdatePopup = true;
        state.sdk.extractedVersion = extractedVersion;
    } else {
        std::cout << "SDKManager: SDK is up to date (Version: " << currentVersion << ")" << "\n";
        ScanExtractedSDK(state);
    }
#endif
}

void SDKManager::ExtractBundledSDK() {
#if defined(TARGET_WASM)
    return; // Not applicable for WASM
#else
    if (sdk_zip_data.empty()) {
        return;
    }

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (mz_zip_reader_init_mem(&zip_archive, sdk_zip_data.data(), sdk_zip_data.size(), 0) == 0) {
        std::cerr << "SDKManager: Failed to initialize ZIP reader from bundled data" << "\n";
        return;
    }

    const fs::path sdkDir = GetSDKDir();
    fs::create_directories(sdkDir);

    int extracted_count = 0;
    int file_count = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat file_stat;
        if (mz_zip_reader_file_stat(&zip_archive, static_cast<mz_uint>(i), &file_stat) == 0) {
            continue;
        }

        std::string filename = file_stat.m_filename;
        if (filename.starts_with("output/")) {
            filename = filename.substr(7); // Strip "output/"
        }

        if (filename.empty()) {
            continue;
        }

        fs::path target_path = sdkDir / filename;

        if (mz_zip_reader_is_file_a_directory(&zip_archive, static_cast<mz_uint>(i)) != 0) {
            fs::create_directories(target_path);
            continue;
        }

        // Ensure parent directories exist
        if (target_path.has_parent_path()) {
            fs::create_directories(target_path.parent_path());
        }

        // Extract file (this will overwrite if it exists)
        if (mz_zip_reader_extract_to_file(&zip_archive, static_cast<mz_uint>(i), target_path.string().c_str(), 0) ==
            0) {
            std::cerr << "SDKManager: Failed to extract " << target_path << "\n";
        } else {
            extracted_count++;
        }
    }

    std::cout << "SDKManager: Extracted " << extracted_count << " files to SDK/ directory" << "\n";
    mz_zip_reader_end(&zip_archive);

    WriteManifest();
#endif
}

void SDKManager::DeleteSDK() {
#if !defined(TARGET_WASM)
    const fs::path sdkDir = GetSDKDir();
    if (fs::exists(sdkDir)) {
        try {
            fs::remove_all(sdkDir);
            std::cout << "SDKManager: SDK folder deleted." << "\n";
        } catch (const std::exception& e) {
            std::cerr << "SDKManager: Error deleting SDK folder: " << e.what() << "\n";
        }
    }
#endif
}

void SDKManager::WriteManifest() {
#if !defined(TARGET_WASM)
    try {
        nlohmann::json manifest;
        manifest["program_version"] = std::string(PROJECT_VERSION);
        std::ofstream ofs(GetSDKDir() / "manifest.json");
        if (ofs.is_open()) {
            ofs << manifest.dump(4);
        }
    } catch (...) {
        std::cerr << "SDKManager: Failed to write manifest.json" << "\n";
    }
#endif
}

std::string SDKManager::GetExtractedVersion() {
#if defined(TARGET_WASM)
    return "";
#else
    try {
        const fs::path manifestPath = GetSDKDir() / "manifest.json";
        if (!fs::exists(manifestPath)) {
            return "";
        }
        std::ifstream ifs(manifestPath);
        if (!ifs.is_open()) {
            return "";
        }
        nlohmann::json manifest = nlohmann::json::parse(ifs);
        return manifest.value("program_version", "");
    } catch (...) {
        return "";
    }
#endif
}

void SDKManager::ScanExtractedSDK(AppState& state) {
    state.sdk.roms.clear();
    state.sdk.midis.clear();
    state.sdk.vrams.clear();

    auto scan_dir = [](const fs::path& dir, std::vector<std::string>& list, const std::string& ext) {
        if (!fs::exists(dir)) {
            return;
        }
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                list.push_back(entry.path().filename().string());
            }
        }
        std::ranges::sort(list);
    };

    const fs::path sdkDir = GetSDKDir();
    scan_dir(sdkDir / "cartridge", state.sdk.roms, ".65c");
    scan_dir(sdkDir / "midi", state.sdk.midis, ".65c");
    scan_dir(sdkDir / "vram", state.sdk.vrams, ".65c");

    if (state.sdk.roms.empty() && state.sdk.midis.empty() && state.sdk.vrams.empty()) {
        std::cerr << "SDKManager: No SDK resources found in SDK/ subfolders" << "\n";
    } else {
        std::cout << "SDKManager: Found " << state.sdk.roms.size() << " ROMs, " << state.sdk.midis.size()
                  << " MIDIs, " << state.sdk.vrams.size() << " VRAMs" << "\n";
    }

    state.sdk.loaded = !state.sdk.roms.empty() || !state.sdk.midis.empty() || !state.sdk.vrams.empty();
}

std::string SDKManager::ResolveLibPath(const std::string& lib) {
    static const std::array<const char*, 10> searchDirs = {
        "sdk/microdos/Libs/",
        "SDK/sdk/microdos/Libs/",
        "output/SDK/sdk/microdos/Libs/",
        "../sdk/microdos/Libs/",
        "../../sdk/microdos/Libs/",
        "sdk/src/Libs/",
        "SDK/sdk/src/Libs/",
        "output/SDK/sdk/src/Libs/",
        "../sdk/src/Libs/",
        "../../sdk/src/Libs/"
    };
    for (const auto* dir : searchDirs) {
        std::string path = std::string(dir) + lib;
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
#ifndef TARGET_WASM
    const std::string vfsDir = CompilerVFS::GetTempDir();
    if (!vfsDir.empty()) {
        std::string vfsPath = vfsDir + "/include/Libs/" + lib;
        if (std::filesystem::exists(vfsPath)) {
            return vfsPath;
        }
    }
#endif
    return {};
}

std::vector<std::string> SDKManager::GetAvailableLibraries(IDEMode mode) {
    std::vector<std::string> libs;
    std::unordered_set<std::string> seen;

    auto scanDir = [&](const std::string& dirPath) {
        std::error_code ec;
        std::vector<std::string> files;
        if (std::filesystem::exists(dirPath, ec) && std::filesystem::is_directory(dirPath, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(dirPath, ec)) {
                if (entry.is_regular_file(ec) && entry.path().extension() == ".h") {
                    files.push_back(entry.path().filename().string());
                }
            }
            std::sort(files.begin(), files.end());
        }
        return files;
    };

    if (mode == IDEMode::MicroDOS) {
        const std::array<const char*, 5> microdosDirs = {
            "sdk/microdos/Libs", "SDK/sdk/microdos/Libs", "output/SDK/sdk/microdos/Libs", "../sdk/microdos/Libs", "../../sdk/microdos/Libs"
        };
        bool scannedMicrodos = false;
        for (const auto* dir : microdosDirs) {
            auto files = scanDir(dir);
            if (!files.empty()) {
                scannedMicrodos = true;
                for (const auto& file : files) {
                    if (seen.find(file) == seen.end()) {
                        seen.insert(file);
                        libs.push_back(file);
                    }
                }
            }
        }

        if (!scannedMicrodos) {
            static const std::array<const char*, 3> microdosLibs = {"app-bios.h", "System.h", "tui.h"};
            for (const auto* lib : microdosLibs) {
                if (seen.find(lib) == seen.end()) {
                    seen.insert(lib);
                    libs.emplace_back(lib);
                }
            }
        }
    }

    const std::array<const char*, 5> standardDirs = {
        "sdk/src/Libs", "SDK/sdk/src/Libs", "output/SDK/sdk/src/Libs", "../sdk/src/Libs", "../../sdk/src/Libs"
    };
    bool scannedAny = false;
    for (const auto* dir : standardDirs) {
        auto files = scanDir(dir);
        if (!files.empty()) {
            scannedAny = true;
            for (const auto& file : files) {
                if (seen.find(file) == seen.end()) {
                    seen.insert(file);
                    libs.push_back(file);
                }
            }
        }
    }

    if (!scannedAny) {
        static const std::array<const char*, 8> standardLibs = {
            "BIOS.h", "GPU.h", "GPUDoubleBuffer.h", "LCD.h", "NET.h", "SD.h", "SID.h", "VIA.h"
        };
        for (const auto* lib : standardLibs) {
            if (seen.find(lib) == seen.end()) {
                seen.insert(lib);
                libs.emplace_back(lib);
            }
        }
    }

    return libs;
}

} // namespace Control
