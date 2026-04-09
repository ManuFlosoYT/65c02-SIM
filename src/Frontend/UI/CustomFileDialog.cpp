#include "Frontend/UI/CustomFileDialog.h"

namespace Frontend {

CustomFileDialog* CustomFileDialog::Instance() {
    static CustomFileDialog instance;
    return &instance;
}

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

bool CustomFileDialog::IsOpened() {
    return ImGuiFileDialog::Instance()->IsOpened();
}

bool CustomFileDialog::IsOk() {
    return ImGuiFileDialog::Instance()->IsOk();
}

std::string CustomFileDialog::GetFilePathName() {
    return ImGuiFileDialog::Instance()->GetFilePathName();
}

void CustomFileDialog::Close() {
    ImGuiFileDialog::Instance()->Close();
}

} // namespace Frontend
