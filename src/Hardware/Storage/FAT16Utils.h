#pragma once

#include <string>

namespace Hardware::Storage {

bool CreateFAT16Image(const std::string& path);
bool CreateFAT16FromFolder(const std::string& folderPath, const std::string& outputPath);

}  // namespace Hardware::Storage

