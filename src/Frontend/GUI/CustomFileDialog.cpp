#include "Frontend/GUI/CustomFileDialog.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif

namespace Frontend {

void NativeFileDialogImpl::OpenDialog(const std::string& vKey, const std::string& vTitle, const char* vFilters,
                                      const std::string& vPath, const std::string& vFileName,
                                      int vCountSelectionMax, void* vUserDatas,
                                      ImGuiFileDialogFlags vFlags) {
#ifndef TARGET_WASM
    IGFD::FileDialogConfig config;
    config.path = vPath;
    config.fileName = vFileName;
    config.countSelectionMax = vCountSelectionMax;
    config.userDatas = vUserDatas;
    config.flags = vFlags;
    ImGuiFileDialog::Instance()->OpenDialog(vKey, vTitle, vFilters, config);
#else
    (void)vKey;
    (void)vTitle;
    (void)vFilters;
    (void)vPath;
    (void)vFileName;
    (void)vCountSelectionMax;
    (void)vUserDatas;
    (void)vFlags;
#endif
}

bool NativeFileDialogImpl::Display(const std::string& vKey, ImGuiWindowFlags vFlags, ImVec2 vMinSize, ImVec2 vMaxSize) {
#ifndef TARGET_WASM
    return ImGuiFileDialog::Instance()->Display(vKey, vFlags, vMinSize, vMaxSize);
#else
    (void)vKey;
    (void)vFlags;
    (void)vMinSize;
    (void)vMaxSize;
    return false;
#endif
}

bool NativeFileDialogImpl::IsOpened(const std::string& vKey) {
#ifndef TARGET_WASM
    if (vKey.empty()) {
        return ImGuiFileDialog::Instance()->IsOpened();
    }
    return ImGuiFileDialog::Instance()->IsOpened(vKey);
#else
    (void)vKey;
    return false;
#endif
}

bool NativeFileDialogImpl::IsOk() {
#ifndef TARGET_WASM
    return ImGuiFileDialog::Instance()->IsOk();
#else
    return false;
#endif
}

std::string NativeFileDialogImpl::GetFilePathName() {
#ifndef TARGET_WASM
    return ImGuiFileDialog::Instance()->GetFilePathName();
#else
    return {};
#endif
}

std::string NativeFileDialogImpl::GetCurrentPath() {
#ifndef TARGET_WASM
    return ImGuiFileDialog::Instance()->GetCurrentPath();
#else
    return {};
#endif
}

void NativeFileDialogImpl::Close() {
#ifndef TARGET_WASM
    ImGuiFileDialog::Instance()->Close();
#endif
}

void WasmFileDialogImpl::OpenDialog(const std::string& vKey, const std::string& vTitle, const char* vFilters,
                                    const std::string& vPath, const std::string& vFileName,
                                    int vCountSelectionMax, void* vUserDatas,
                                    ImGuiFileDialogFlags vFlags) {
#ifdef TARGET_WASM
    WebFileUtils::open_browser_file_picker(vFilters ? vFilters : "");
#else
    (void)vKey;
    (void)vTitle;
    (void)vFilters;
    (void)vPath;
    (void)vFileName;
    (void)vCountSelectionMax;
    (void)vUserDatas;
    (void)vFlags;
#endif
}

bool WasmFileDialogImpl::Display(const std::string& vKey, ImGuiWindowFlags vFlags, ImVec2 vMinSize, ImVec2 vMaxSize) {
    (void)vKey;
    (void)vFlags;
    (void)vMinSize;
    (void)vMaxSize;
    return false;
}

bool WasmFileDialogImpl::IsOpened(const std::string& vKey) {
    (void)vKey;
    return false;
}

bool WasmFileDialogImpl::IsOk() {
    return false;
}

std::string WasmFileDialogImpl::GetFilePathName() {
    return {};
}

std::string WasmFileDialogImpl::GetCurrentPath() {
    return {};
}

void WasmFileDialogImpl::Close() {
}

void CustomFileDialog::OpenDialog(const std::string& vKey, const std::string& vTitle, const char* vFilters,
                                  const std::string& vPath, const std::string& vFileName,
                                  int vCountSelectionMax, void* vUserDatas,
                                  ImGuiFileDialogFlags vFlags) {
#ifdef TARGET_WASM
    WasmFileDialogImpl::OpenDialog(vKey, vTitle, vFilters, vPath, vFileName, vCountSelectionMax, vUserDatas, vFlags);
#else
    NativeFileDialogImpl::OpenDialog(vKey, vTitle, vFilters, vPath, vFileName, vCountSelectionMax, vUserDatas, vFlags);
#endif
}

bool CustomFileDialog::Display(const std::string& vKey, ImGuiWindowFlags vFlags, ImVec2 vMinSize, ImVec2 vMaxSize) {
#ifdef TARGET_WASM
    return WasmFileDialogImpl::Display(vKey, vFlags, vMinSize, vMaxSize);
#else
    return NativeFileDialogImpl::Display(vKey, vFlags, vMinSize, vMaxSize);
#endif
}

bool CustomFileDialog::IsOpened(const std::string& vKey) {
#ifdef TARGET_WASM
    return WasmFileDialogImpl::IsOpened(vKey);
#else
    return NativeFileDialogImpl::IsOpened(vKey);
#endif
}

bool CustomFileDialog::IsOk() {
#ifdef TARGET_WASM
    return WasmFileDialogImpl::IsOk();
#else
    return NativeFileDialogImpl::IsOk();
#endif
}

std::string CustomFileDialog::GetFilePathName() {
#ifdef TARGET_WASM
    return WasmFileDialogImpl::GetFilePathName();
#else
    return NativeFileDialogImpl::GetFilePathName();
#endif
}

std::string CustomFileDialog::GetCurrentPath() {
#ifdef TARGET_WASM
    return WasmFileDialogImpl::GetCurrentPath();
#else
    return NativeFileDialogImpl::GetCurrentPath();
#endif
}

void CustomFileDialog::Close() {
#ifdef TARGET_WASM
    WasmFileDialogImpl::Close();
#else
    NativeFileDialogImpl::Close();
#endif
}

} // namespace Frontend
