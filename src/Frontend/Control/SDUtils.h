#pragma once

#include "Frontend/Control/AppState.h"
#include <string>

namespace GUI {

bool IsSDCardEnabled(Control::AppState& state);
bool CreateFAT16Image(const std::string& path);
bool MountFolder(Control::AppState& state, const std::string& folderPath, bool allowDeletion = false);
bool CreateFAT16FromFolder(const std::string& folderPath, const std::string& outputPath);
bool CreateMountSentinel(const std::string& folderPath);

}  // namespace GUI
