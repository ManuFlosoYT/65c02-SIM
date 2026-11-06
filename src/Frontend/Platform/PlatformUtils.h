#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

namespace Frontend::Platform {

bool OpenURL(std::string_view url);
std::filesystem::path GetExecutableDirectory();
char GetEnvPathDelimiter();
std::string AppendExecutableExtension(std::string_view binaryName);
std::string CaptureStackTrace();

void InstallCrashHandler();
void EnsurePlatformDependencies();
bool RunProtected(const std::function<void()>& fn, std::string& outErrorMessage);

}  // namespace Frontend::Platform
