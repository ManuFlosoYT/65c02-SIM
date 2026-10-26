#include "Frontend/Control/SDKInstaller.h"

#include <miniz.h>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#if !defined(TARGET_WASM)
#include "sdk_zip.h"
#endif

namespace fs = std::filesystem;

namespace Control {

fs::path SDKInstaller::GetSDKDir() {
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

bool SDKInstaller::IsInstalled() {
    return fs::exists(GetSDKDir());
}

void SDKInstaller::ExtractBundledSDK() {
#if defined(TARGET_WASM)
    return;
#else
    if (sdk_zip_data.empty()) {
        return;
    }

    mz_zip_archive zip_archive;
    std::memset(&zip_archive, 0, sizeof(zip_archive));

    if (mz_zip_reader_init_mem(&zip_archive, sdk_zip_data.data(), sdk_zip_data.size(), 0) == 0) {
        std::cerr << "SDKManager: Failed to initialize ZIP reader from bundled data\n";
        return;
    }

    const fs::path sdkDir = GetSDKDir();
    fs::create_directories(sdkDir);

    int extracted_count = 0;
    int file_count = static_cast<int>(mz_zip_reader_get_num_files(&zip_archive));
    for (int i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat file_stat;
        if (mz_zip_reader_file_stat(&zip_archive, static_cast<mz_uint>(i), &file_stat) == 0) {
            continue;
        }

        std::string filename = file_stat.m_filename;
        if (filename.starts_with("output/")) {
            filename = filename.substr(7);
        }

        if (filename.empty()) {
            continue;
        }

        fs::path target_path = sdkDir / filename;

        if (mz_zip_reader_is_file_a_directory(&zip_archive, static_cast<mz_uint>(i)) != 0) {
            fs::create_directories(target_path);
            continue;
        }

        if (target_path.has_parent_path()) {
            fs::create_directories(target_path.parent_path());
        }

        if (mz_zip_reader_extract_to_file(&zip_archive, static_cast<mz_uint>(i), target_path.string().c_str(), 0) == 0) {
            std::cerr << "SDKManager: Failed to extract " << target_path << "\n";
        } else {
            extracted_count++;
        }
    }

    std::cout << "SDKManager: Extracted " << extracted_count << " files to SDK/ directory\n";
    mz_zip_reader_end(&zip_archive);

    WriteManifest();
#endif
}

void SDKInstaller::DeleteSDK() {
#if !defined(TARGET_WASM)
    const fs::path sdkDir = GetSDKDir();
    if (fs::exists(sdkDir)) {
        try {
            fs::remove_all(sdkDir);
            std::cout << "SDKManager: SDK folder deleted.\n";
        } catch (const std::exception& e) {
            std::cerr << "SDKManager: Error deleting SDK folder: " << e.what() << "\n";
        }
    }
#endif
}

void SDKInstaller::WriteManifest() {
#if !defined(TARGET_WASM)
    try {
        nlohmann::json manifest;
        manifest["program_version"] = std::string(PROJECT_VERSION);
        std::ofstream ofs(GetSDKDir() / "manifest.json");
        if (ofs.is_open()) {
            ofs << manifest.dump(4);
        }
    } catch (...) {
        std::cerr << "SDKManager: Failed to write manifest.json\n";
    }
#endif
}

std::string SDKInstaller::GetExtractedVersion() {
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

} // namespace Control
