#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Hardware {

class VirtualFAT16Synchronizer {
public:
    static std::filesystem::path PathFromUtf8(std::string_view str);
    static bool IsProtectedHostPath(const std::filesystem::path& canPath);
    static std::string GenerateSFN(const std::string& filename, std::set<std::string>& used_sfns);

    static bool Build(std::vector<uint8_t>& virtual_disk,
                      const std::filesystem::path& hostFolder,
                      std::unordered_map<std::string, std::filesystem::path>& sfnToHostMap);

    static void Extract(const std::vector<uint8_t>& virtual_disk,
                        const std::filesystem::path& hostFolder,
                        const std::unordered_map<std::string, std::filesystem::path>& sfnToHostMap,
                        bool allowDeletion);
};

}  // namespace Hardware
