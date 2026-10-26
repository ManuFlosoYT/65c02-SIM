#include "Frontend/Control/SDKManager.h"
#include "Frontend/Control/SDKInstaller.h"
#include "Frontend/Control/SDKCatalogScanner.h"
#include "Frontend/Control/AppState.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <unordered_set>
#include <algorithm>

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerVFS.h"
#endif

namespace Control {

void SDKManager::CheckSDKStatus(AppState& state) {
#if defined(TARGET_WASM)
    return;
#else
    if (!SDKInstaller::IsInstalled()) {
        std::cout << "SDKManager: SDK folder missing. Performing initial extraction..." << "\n";
        SDKInstaller::ExtractBundledSDK();
        SDKCatalogScanner::ScanExtractedSDK(state);
        return;
    }

    std::string extractedVersion = SDKInstaller::GetExtractedVersion();
    std::string currentVersion = std::string(PROJECT_VERSION);

    if (extractedVersion.empty() || extractedVersion != currentVersion) {
        std::cout << "SDKManager: SDK version mismatch (Extracted: " << (extractedVersion.empty() ? "None" : extractedVersion)
                  << ", Current: " << currentVersion << "). Queuing update popup..." << "\n";
        state.sdk.showUpdatePopup = true;
        state.sdk.extractedVersion = extractedVersion;
    } else {
        std::cout << "SDKManager: SDK is up to date (Version: " << currentVersion << ")" << "\n";
        SDKCatalogScanner::ScanExtractedSDK(state);
    }
#endif
}

void SDKManager::ExtractBundledSDK() {
    SDKInstaller::ExtractBundledSDK();
}

void SDKManager::DeleteSDK() {
    SDKInstaller::DeleteSDK();
}

void SDKManager::ScanExtractedSDK(AppState& state) {
    SDKCatalogScanner::ScanExtractedSDK(state);
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
