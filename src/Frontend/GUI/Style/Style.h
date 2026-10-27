#pragma once

#include <imgui.h>
#include <string>
#include <string_view>

namespace Theme {
inline constexpr ImVec4 WindowBg{0.067F, 0.075F, 0.102F, 1.00F};
inline constexpr ImVec4 DeepBg{0.039F, 0.047F, 0.063F, 1.00F};
inline constexpr ImVec4 Surface{0.110F, 0.125F, 0.169F, 1.00F};
inline constexpr ImVec4 SurfaceHover{0.157F, 0.188F, 0.259F, 1.00F};
inline constexpr ImVec4 BorderSubtle{0.149F, 0.169F, 0.220F, 1.00F};
inline constexpr ImVec4 AccentPrimary{0.220F, 0.741F, 0.973F, 1.00F};
inline constexpr ImVec4 AccentActive{0.063F, 0.725F, 0.506F, 1.00F};
inline constexpr ImVec4 RunBg{0.024F, 0.306F, 0.231F, 1.00F};
inline constexpr ImVec4 RecIdle{0.271F, 0.102F, 0.102F, 1.00F};
inline constexpr ImVec4 RecActive{0.937F, 0.267F, 0.267F, 1.00F};
inline constexpr ImVec4 TextMain{0.886F, 0.910F, 0.941F, 1.00F};
inline constexpr ImVec4 TextSecondary{0.392F, 0.455F, 0.545F, 1.00F};

inline constexpr ImU32 LcdPetroleumBg = IM_COL32(7, 23, 18, 255);
inline constexpr ImU32 LcdBevelBorder = IM_COL32(22, 56, 44, 255);
inline constexpr ImU32 LcdCellInactive = IM_COL32(13, 38, 30, 255);
inline constexpr ImU32 LcdActivePixel = IM_COL32(16, 185, 129, 255);

ImVec4 GetDeviceColor(std::string_view name);
inline ImVec4 GetDeviceColor(const std::string& name) { return GetDeviceColor(std::string_view(name)); }
}  // namespace Theme

void SetupImGuiStyle();
void SetupModernStyle();
void SetupFont();
