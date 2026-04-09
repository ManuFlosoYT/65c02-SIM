#include "Frontend/Control/SDKManager.h"
#include "Frontend/Control/AppState.h"

#include <miniz.h>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>

#if !defined(TARGET_WASM)
#include "sdk_zip.h"
#endif

namespace fs = std::filesystem;

namespace Control {

void SDKManager::CheckSDKStatus(AppState& state) {
#if defined(TARGET_WASM)
    return;
#else
    if (!fs::exists("SDK")) {
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

    fs::create_directories("SDK");

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

        fs::path target_path = fs::path("SDK") / filename;

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
    if (fs::exists("SDK")) {
        try {
            fs::remove_all("SDK");
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
        std::ofstream ofs("SDK/manifest.json");
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
        if (!fs::exists("SDK/manifest.json")) {
            return "";
        }
        std::ifstream ifs("SDK/manifest.json");
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

    scan_dir("SDK/cartridge", state.sdk.roms, ".65c");
    scan_dir("SDK/midi", state.sdk.midis, ".65c");
    scan_dir("SDK/vram", state.sdk.vrams, ".65c");

    if (state.sdk.roms.empty() && state.sdk.midis.empty() && state.sdk.vrams.empty()) {
        std::cerr << "SDKManager: No SDK resources found in SDK/ subfolders" << "\n";
    } else {
        std::cout << "SDKManager: Found " << state.sdk.roms.size() << " ROMs, " << state.sdk.midis.size()
                  << " MIDIs, " << state.sdk.vrams.size() << " VRAMs" << "\n";
    }

    state.sdk.loaded = !state.sdk.roms.empty() || !state.sdk.midis.empty() || !state.sdk.vrams.empty();
}

} // namespace Control
