#pragma once

#include "Frontend/Control/AppState.h"
#include <string>

namespace GUI {

bool IsSDCardEnabled(Control::AppState& state);
bool CreateFAT16Image(const std::string& path);

}  // namespace GUI
