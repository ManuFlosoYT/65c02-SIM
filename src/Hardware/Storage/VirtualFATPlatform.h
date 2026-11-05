#pragma once

#include <ctime>
#include <filesystem>
#include <string_view>
#include <vector>

namespace Hardware::Storage::Platform {

bool ComponentEquals(std::string_view a, std::string_view b);
bool SafeLocalTime(const std::time_t* time, std::tm* result);
std::vector<std::filesystem::path> GetProtectedDirectories();

}  // namespace Hardware::Storage::Platform
