#pragma once

#include <ImGuiFileDialog.h>
#include <string>

namespace Frontend {

class NativeFileDialogImpl {
public:
    static void OpenDialog(const std::string& vKey, const std::string& vTitle, const char* vFilters,
                           const std::string& vPath = ".", const std::string& vFileName = "",
                           int vCountSelectionMax = 1, void* vUserDatas = nullptr,
                           ImGuiFileDialogFlags vFlags = ImGuiFileDialogFlags_None);

    static bool Display(const std::string& vKey, ImGuiWindowFlags vFlags = ImGuiWindowFlags_NoCollapse, 
                        ImVec2 vMinSize = ImVec2(400, 300), ImVec2 vMaxSize = ImVec2(32767, 32767));

    [[nodiscard]] static bool IsOpened(const std::string& vKey = "");
    [[nodiscard]] static bool IsOk();
    [[nodiscard]] static std::string GetFilePathName();
    [[nodiscard]] static std::string GetCurrentPath();
    static void Close();
};

class WasmFileDialogImpl {
public:
    static void OpenDialog(const std::string& vKey, const std::string& vTitle, const char* vFilters,
                           const std::string& vPath = ".", const std::string& vFileName = "",
                           int vCountSelectionMax = 1, void* vUserDatas = nullptr,
                           ImGuiFileDialogFlags vFlags = ImGuiFileDialogFlags_None);

    static bool Display(const std::string& vKey, ImGuiWindowFlags vFlags = ImGuiWindowFlags_NoCollapse, 
                        ImVec2 vMinSize = ImVec2(400, 300), ImVec2 vMaxSize = ImVec2(32767, 32767));

    [[nodiscard]] static bool IsOpened(const std::string& vKey = "");
    [[nodiscard]] static bool IsOk();
    [[nodiscard]] static std::string GetFilePathName();
    [[nodiscard]] static std::string GetCurrentPath();
    static void Close();
};

class CustomFileDialog {
public:
    static void OpenDialog(const std::string& vKey, const std::string& vTitle, const char* vFilters,
                           const std::string& vPath = ".", const std::string& vFileName = "",
                           int vCountSelectionMax = 1, void* vUserDatas = nullptr,
                           ImGuiFileDialogFlags vFlags = ImGuiFileDialogFlags_None);

    static bool Display(const std::string& vKey, ImGuiWindowFlags vFlags = ImGuiWindowFlags_NoCollapse, 
                        ImVec2 vMinSize = ImVec2(400, 300), ImVec2 vMaxSize = ImVec2(32767, 32767));

    [[nodiscard]] static bool IsOpened(const std::string& vKey = "");
    [[nodiscard]] static bool IsOk();
    [[nodiscard]] static std::string GetFilePathName();
    [[nodiscard]] static std::string GetCurrentPath();
    static void Close();

private:
    CustomFileDialog() = default;
};

} // namespace Frontend
