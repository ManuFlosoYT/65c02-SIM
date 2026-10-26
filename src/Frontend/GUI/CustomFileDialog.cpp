#include "Frontend/GUI/CustomFileDialog.h"

namespace Frontend {

void CustomFileDialog::OpenDialog(const std::string& vKey, const std::string& vTitle, const char* vFilters,
                                  const std::string& vPath, const std::string& vFileName,
                                  int vCountSelectionMax, void* vUserDatas,
                                  ImGuiFileDialogFlags vFlags) {
    IGFD::FileDialogConfig config;
    config.path = vPath;
    config.fileName = vFileName;
    config.countSelectionMax = vCountSelectionMax;
    config.userDatas = vUserDatas;
    config.flags = vFlags;
    ImGuiFileDialog::Instance()->OpenDialog(vKey, vTitle, vFilters, config);
}

bool CustomFileDialog::Display(const std::string& vKey, ImGuiWindowFlags vFlags, ImVec2 vMinSize, ImVec2 vMaxSize) {
    return ImGuiFileDialog::Instance()->Display(vKey, vFlags, vMinSize, vMaxSize);
}

bool CustomFileDialog::IsOpened(const std::string& vKey) {
    if (vKey.empty()) {
        return ImGuiFileDialog::Instance()->IsOpened();
    }
    return ImGuiFileDialog::Instance()->IsOpened(vKey);
}

bool CustomFileDialog::IsOk() {
    return ImGuiFileDialog::Instance()->IsOk();
}

std::string CustomFileDialog::GetFilePathName() {
    return ImGuiFileDialog::Instance()->GetFilePathName();
}

std::string CustomFileDialog::GetCurrentPath() {
    return ImGuiFileDialog::Instance()->GetCurrentPath();
}

void CustomFileDialog::Close() {
    ImGuiFileDialog::Instance()->Close();
}

} // namespace Frontend
