#pragma once

#include <cstddef>
#include <cstdint>

namespace Frontend {

class BrowserFileBridge {
public:
    static void OpenFilePicker(const char* accept);
    static void DownloadFile(const char* filename, const uint8_t* data, size_t size);
    static void FetchFile(const char* url, const char* filename);
    static void ShowToast(const char* message);
    static void CheckUrlHash();
};

}  // namespace Frontend
