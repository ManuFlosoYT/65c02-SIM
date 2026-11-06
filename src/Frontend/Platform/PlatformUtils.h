#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Frontend::Platform {

bool OpenURL(std::string_view url);
std::filesystem::path GetExecutableDirectory();
char GetEnvPathDelimiter();
std::string AppendExecutableExtension(std::string_view binaryName);
std::string CaptureStackTrace();

}  // namespace Frontend::Platform
