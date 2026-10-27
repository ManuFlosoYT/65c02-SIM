#include "Style.h"
#include <cmath>
#include <cstdint>
#include "Frontend/GUI/Style/FontLoader.h"

void SetupImGuiStyle() {
    // Moonlight style by deathsu/madam-herta
    // https://github.com/Madam-Herta/Moonlight/
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0F;
    style.DisabledAlpha = 0.5F;
    style.WindowPadding = ImVec2(12.0F, 12.0F);
    style.WindowRounding = 11.5F;
    style.WindowBorderSize = 1.0F;
    style.WindowMinSize = ImVec2(20.0F, 20.0F);
    style.WindowTitleAlign = ImVec2(0.5F, 0.5F);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ChildRounding = 0.0F;
    style.ChildBorderSize = 1.0F;
    style.PopupRounding = 0.0F;
    style.PopupBorderSize = 1.0F;
    style.FramePadding = ImVec2(20.0F, 3.400000095367432F);
    style.FrameRounding = 11.89999961853027F;
    style.FrameBorderSize = 1.0F;
    style.ItemSpacing = ImVec2(4.300000190734863F, 5.5F);
    style.ItemInnerSpacing = ImVec2(7.099999904632568F, 1.799999952316284F);
    style.CellPadding = ImVec2(12.10000038146973F, 9.199999809265137F);
    style.IndentSpacing = 0.0F;
    style.ColumnsMinSpacing = 4.900000095367432F;
    style.ScrollbarSize = 11.60000038146973F;
    style.ScrollbarRounding = 15.89999961853027F;
    style.GrabMinSize = 3.700000047683716F;
    style.GrabRounding = 20.0F;
    style.TabRounding = 0.0F;
    style.TabBorderSize = 0.0F;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5F, 0.5F);



    style.SelectableTextAlign = ImVec2(0.0F, 0.0F);

    style.Colors[ImGuiCol_Text] = Theme::TextMain;
    style.Colors[ImGuiCol_TextDisabled] = Theme::TextSecondary;
    style.Colors[ImGuiCol_WindowBg] = Theme::WindowBg;
    style.Colors[ImGuiCol_ChildBg] = Theme::DeepBg;
    style.Colors[ImGuiCol_PopupBg] = Theme::WindowBg;
    style.Colors[ImGuiCol_Border] = Theme::BorderSubtle;
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_FrameBg] = Theme::DeepBg;
    style.Colors[ImGuiCol_FrameBgHovered] = Theme::SurfaceHover;
    style.Colors[ImGuiCol_FrameBgActive] = Theme::SurfaceHover;
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.047F, 0.055F, 0.071F, 1.0F);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.047F, 0.055F, 0.071F, 1.0F);
    style.Colors[ImGuiCol_TitleBgCollapsed] =
        ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248F, 0.105882354080677F, 0.1215686276555061F, 1.0F);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776F, 0.05490196123719215F, 0.07058823853731155F, 1.0F);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989F, 0.1333333402872086F, 0.1490196138620377F, 1.0F);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] =
        ImVec4(0.1568627506494522F, 0.168627455830574F, 0.1921568661928177F, 1.0F);
    style.Colors[ImGuiCol_ScrollbarGrabActive] =
        ImVec4(0.1176470592617989F, 0.1333333402872086F, 0.1490196138620377F, 1.0F);
    style.Colors[ImGuiCol_CheckMark] = Theme::AccentPrimary;
    style.Colors[ImGuiCol_SliderGrab] = Theme::AccentPrimary;
    style.Colors[ImGuiCol_SliderGrabActive] = Theme::AccentPrimary;
    style.Colors[ImGuiCol_Button] = Theme::Surface;
    style.Colors[ImGuiCol_ButtonHovered] = Theme::SurfaceHover;
    style.Colors[ImGuiCol_ButtonActive] = Theme::SurfaceHover;
    style.Colors[ImGuiCol_Header] = Theme::Surface;
    style.Colors[ImGuiCol_HeaderHovered] = Theme::SurfaceHover;
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_Separator] = Theme::BorderSubtle;
    style.Colors[ImGuiCol_SeparatorHovered] =
        ImVec4(0.1568627506494522F, 0.1843137294054031F, 0.250980406999588F, 1.0F);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522F, 0.1843137294054031F, 0.250980406999588F, 1.0F);
    style.Colors[ImGuiCol_ResizeGrip] = Theme::BorderSubtle;
    style.Colors[ImGuiCol_ResizeGripHovered] = Theme::AccentPrimary;
    style.Colors[ImGuiCol_ResizeGripActive] = Theme::AccentPrimary;
    style.Colors[ImGuiCol_Tab] = Theme::Surface;
    style.Colors[ImGuiCol_TabHovered] = Theme::SurfaceHover;
    style.Colors[ImGuiCol_TabSelected] = Theme::SurfaceHover;
    style.Colors[ImGuiCol_TabSelectedOverline] = Theme::AccentPrimary;
    style.Colors[ImGuiCol_TabDimmed] = Theme::Surface;
    style.Colors[ImGuiCol_TabDimmedSelected] = Theme::SurfaceHover;
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = Theme::AccentPrimary;
    style.Colors[ImGuiCol_PlotLines] = Theme::AccentPrimary;
    style.Colors[ImGuiCol_PlotLinesHovered] =
        ImVec4(0.03921568766236305F, 0.9803921580314636F, 0.9803921580314636F, 1.0F);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8841201663017273F, 0.7941429018974304F, 0.5615870356559753F, 1.0F);
    style.Colors[ImGuiCol_PlotHistogramHovered] =
        ImVec4(0.9570815563201904F, 0.9570719599723816F, 0.9570761322975159F, 1.0F);
    style.Colors[ImGuiCol_TableHeaderBg] =
        ImVec4(0.0470588244497776F, 0.05490196123719215F, 0.07058823853731155F, 1.0F);
    style.Colors[ImGuiCol_TableBorderStrong] =
        ImVec4(0.0470588244497776F, 0.05490196123719215F, 0.07058823853731155F, 1.0F);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0F, 0.0F, 0.0F, 1.0F);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989F, 0.1333333402872086F, 0.1490196138620377F, 1.0F);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248F, 0.105882354080677F, 0.1215686276555061F, 1.0F);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.9356134533882141F, 0.9356129765510559F, 0.9356223344802856F, 1.0F);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464F, 0.5137255191802979F, 1.0F, 1.0F);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.266094446182251F, 0.2890366911888123F, 1.0F, 1.0F);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464F, 0.5137255191802979F, 1.0F, 1.0F);
    style.Colors[ImGuiCol_NavWindowingDimBg] =
        ImVec4(0.196078434586525F, 0.1764705926179886F, 0.5450980663299561F, 0.501960813999176F);
    style.Colors[ImGuiCol_ModalWindowDimBg] =
        ImVec4(0.196078434586525F, 0.1764705926179886F, 0.5450980663299561F, 0.501960813999176F);
}

void SetupModernStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 8.0F;
    style.FrameRounding = 6.0F;
    style.PopupRounding = 6.0F;
    style.ScrollbarRounding = 6.0F;
    style.GrabRounding = 4.0F;
    style.TabRounding = 6.0F;

    style.WindowBorderSize = 1.0F;
    style.FrameBorderSize = 1.0F;
    style.TabBorderSize = 1.0F;

    style.ItemSpacing = ImVec2(8.0F, 6.0F);
    style.WindowPadding = ImVec2(12.0F, 12.0F);
}

void SetupFont(){
    GUI::FontLoader::SetupFont();
}

namespace Theme {

ImVec4 GetDeviceColor(std::string_view name) {
    uint32_t hash = 0x811c9dc5;
    for (char character : name) {
        hash ^= static_cast<uint8_t>(character);
        hash *= 0x01000193;
    }

    double hue = std::fmod(static_cast<double>(hash) * 0.618033988749895, 1.0);
    double sat = 0.6 + std::fmod(static_cast<double>(hash) * 0.1, 0.3);
    double val = 0.8 + std::fmod(static_cast<double>(hash) * 0.05, 0.2);

    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
    ImGui::ColorConvertHSVtoRGB(static_cast<float>(hue), static_cast<float>(sat), static_cast<float>(val), red, green,
                                blue);
    return {red, green, blue, 0.6F};
}

}  // namespace Theme
