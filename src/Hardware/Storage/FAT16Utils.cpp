#include "Hardware/Storage/FAT16Utils.h"

#include <filesystem>
#include <fstream>
#include <vector>

#include "Hardware/Storage/VirtualFAT16Synchronizer.h"

namespace Hardware::Storage {

bool CreateFAT16Image(const std::string& path) {
    std::vector<uint8_t> disk;
    VirtualFAT16Synchronizer::InitializeDiskLayout(disk);

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(disk.data()), static_cast<std::streamsize>(disk.size()));
    return file.good();
}

bool CreateFAT16FromFolder(const std::string& folderPath, const std::string& outputPath) {
    const std::filesystem::path hostFolder = VirtualFAT16Synchronizer::PathFromUtf8(folderPath);
    if (!std::filesystem::exists(hostFolder) || !std::filesystem::is_directory(hostFolder)) {
        return false;
    }
    std::error_code ec;
    std::filesystem::path canPath = std::filesystem::canonical(hostFolder, ec);
    if (ec || VirtualFAT16Synchronizer::IsProtectedHostPath(canPath)) {
        return false;
    }

    std::vector<uint8_t> disk;
    std::unordered_map<std::string, std::filesystem::path> sfnToHostMap;
    if (!VirtualFAT16Synchronizer::Build(disk, hostFolder, sfnToHostMap)) {
        return false;
    }
    std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(disk.data()), static_cast<std::streamsize>(disk.size()));
    return out.good();
}

}  // namespace Hardware::Storage
