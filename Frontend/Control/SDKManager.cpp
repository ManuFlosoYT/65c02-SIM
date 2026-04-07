#include "Frontend/Control/SDKManager.h"
#include "Frontend/Control/AppState.h"

#include <miniz.h>
#include <filesystem>
#include <iostream>

#if !defined(TARGET_WASM)
#if __has_include("generated/sdk_zip.h")
#include "generated/sdk_zip.h"
#else
// Fallback if not yet generated or for local dev without SDK.zip
static const std::vector<uint8_t> sdk_zip_data = {};
#endif
#endif

namespace fs = std::filesystem;

namespace Control {

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

    int file_count = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat file_stat;
        if (mz_zip_reader_file_stat(&zip_archive, static_cast<mz_uint>(i), &file_stat) == 0) {
            continue;
        }

        if (mz_zip_reader_is_file_a_directory(&zip_archive, static_cast<mz_uint>(i)) != 0) {
            fs::create_directories(file_stat.m_filename);
            continue;
        }

        // Ensure parent directories exist
        fs::path file_path(file_stat.m_filename);
        if (file_path.has_parent_path()) {
            fs::create_directories(file_path.parent_path());
        }

        // Extract file (this will overwrite if it exists)
        if (mz_zip_reader_extract_to_file(&zip_archive, static_cast<mz_uint>(i), file_stat.m_filename, 0) == 0) {
            std::cerr << "SDKManager: Failed to extract " << file_stat.m_filename << "\n";
        }
    }

    mz_zip_reader_end(&zip_archive);
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

    scan_dir("output/cartridge", state.sdk.roms, ".65c");
    scan_dir("output/midi", state.sdk.midis, ".65c");
    scan_dir("output/vram", state.sdk.vrams, ".65c");

    state.sdk.loaded = !state.sdk.roms.empty() || !state.sdk.midis.empty() || !state.sdk.vrams.empty();
}

} // namespace Control
