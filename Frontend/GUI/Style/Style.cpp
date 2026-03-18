#include "Style.h"
#include "CascadiaMono.h"

void SetupImGuiStyle() {
    // Moonlight style by deathsu/madam-herta
    // https://github.com/Madam-Herta/Moonlight/
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0F;
    style.DisabledAlpha = 1.0F;
    style.WindowPadding = ImVec2(12.0F, 12.0F);
    style.WindowRounding = 11.5F;
    style.WindowBorderSize = 0.0F;
    style.WindowMinSize = ImVec2(20.0F, 20.0F);
    style.WindowTitleAlign = ImVec2(0.5F, 0.5F);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ChildRounding = 0.0F;
    style.ChildBorderSize = 1.0F;
    style.PopupRounding = 0.0F;
    style.PopupBorderSize = 1.0F;
    style.FramePadding = ImVec2(20.0F, 3.400000095367432F);
    style.FrameRounding = 11.89999961853027F;
    style.FrameBorderSize = 0.0F;
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

    style.Colors[ImGuiCol_Text] = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317F, 0.3176470696926117F, 0.4509803950786591F, 1.0F); //NOLINT
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09250493347644806F, 0.100297249853611F, 0.1158798336982727F, 1.0F);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522F, 0.168627455830574F, 0.1921568661928177F, 1.0F);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1120669096708298F, 0.1262156516313553F, 0.1545064449310303F, 1.0F);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522F, 0.168627455830574F, 0.1921568661928177F, 1.0F);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1568627506494522F, 0.168627455830574F, 0.1921568661928177F, 1.0F);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776F, 0.05490196123719215F, 0.07058823853731155F, 1.0F);
    style.Colors[ImGuiCol_TitleBgActive] =
        ImVec4(0.0470588244497776F, 0.05490196123719215F, 0.07058823853731155F, 1.0F);
    style.Colors[ImGuiCol_TitleBgCollapsed] =
        ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248F, 0.105882354080677F, 0.1215686276555061F, 1.0F);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776F, 0.05490196123719215F, 0.07058823853731155F, 1.0F);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989F, 0.1333333402872086F, 0.1490196138620377F, 1.0F);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] =
        ImVec4(0.1568627506494522F, 0.168627455830574F, 0.1921568661928177F, 1.0F);
    style.Colors[ImGuiCol_ScrollbarGrabActive] =
        ImVec4(0.1176470592617989F, 0.1333333402872086F, 0.1490196138620377F, 1.0F);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.9725490212440491F, 1.0F, 0.4980392158031464F, 1.0F);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.971993625164032F, 1.0F, 0.4980392456054688F, 1.0F);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0F, 0.7953379154205322F, 0.4980392456054688F, 1.0F);
    style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989F, 0.1333333402872086F, 0.1490196138620377F, 1.0F);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1821731775999069F, 0.1897992044687271F, 0.1974248886108398F, 1.0F);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1545050293207169F, 0.1545048952102661F, 0.1545064449310303F, 1.0F);
    style.Colors[ImGuiCol_Header] = ImVec4(0.1414651423692703F, 0.1629818230867386F, 0.2060086131095886F, 1.0F);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1072951927781105F, 0.107295036315918F, 0.1072961091995239F, 1.0F);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.1293079704046249F, 0.1479243338108063F, 0.1931330561637878F, 1.0F);
    style.Colors[ImGuiCol_SeparatorHovered] =
        ImVec4(0.1568627506494522F, 0.1843137294054031F, 0.250980406999588F, 1.0F);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522F, 0.1843137294054031F, 0.250980406999588F, 1.0F);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1459212601184845F, 0.1459220051765442F, 0.1459227204322815F, 1.0F);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.9725490212440491F, 1.0F, 0.4980392158031464F, 1.0F);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.999999463558197F, 1.0F, 0.9999899864196777F, 1.0F);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989F, 0.1333333402872086F, 0.1490196138620377F, 1.0F);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.1176470592617989F, 0.1333333402872086F, 0.1490196138620377F, 1.0F);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0784313753247261F, 0.08627451211214066F, 0.1019607856869698F, 1.0F);
    style.Colors[ImGuiCol_TabUnfocusedActive] =
        ImVec4(0.1249424293637276F, 0.2735691666603088F, 0.5708154439926147F, 1.0F);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124F, 0.6000000238418579F, 0.7019608020782471F, 1.0F);
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

    style.WindowBorderSize = 0.0F;
    style.FrameBorderSize = 0.0F;

    style.ItemSpacing = ImVec2(8.0F, 6.0F);
    style.WindowPadding = ImVec2(12.0F, 12.0F);
}

void SetupFont(){
    ImGuiIO& imguiio = ImGui::GetIO();
    
    ImFont* mainFont = imguiio.Fonts->AddFontFromMemoryCompressedBase85TTF(
        CascadiaMono_compressed_data_base85,
        14.0F
    );
    
    IM_ASSERT(mainFont != NULL);
}