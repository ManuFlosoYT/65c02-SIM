#pragma once

#include <ImGuiFileDialog.h>
#include <string>

namespace Frontend {

class CustomFileDialog {
public:
    static CustomFileDialog* Instance();

    static void OpenDialog(const std::string& vKey, const std::string& vTitle, const char* vFilters,
                           const std::string& vPath = ".", const std::string& vFileName = "",
                           int vCountSelectionMax = 1, void* vUserDatas = nullptr,
                           ImGuiFileDialogFlags vFlags = ImGuiFileDialogFlags_None);

    static bool Display(const std::string& vKey, ImGuiWindowFlags vFlags = ImGuiWindowFlags_NoCollapse, 
                        ImVec2 vMinSize = ImVec2(800, 600), ImVec2 vMaxSize = ImVec2(32767, 32767));

    [[nodiscard]] static bool IsOpened();
    [[nodiscard]] static bool IsOk();
    [[nodiscard]] static std::string GetFilePathName();
    static void Close();

private:
    CustomFileDialog() = default;
};

} // namespace Frontend
