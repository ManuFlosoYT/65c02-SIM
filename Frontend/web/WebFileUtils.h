#pragma once

#ifdef TARGET_WASM
#include <emscripten.h>
#include <emscripten/em_js.h>
#include <string>
#include <vector>
#include <functional>

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void Wasm_OnFilePicked(const char* filename, const uint8_t* data, int size);
}

namespace WebFileUtils {

// Global callbacks to be called from JS
inline std::function<void(const char*, const uint8_t*, int)> onFilePickedCallback;

void open_browser_file_picker(const char* accept);
void download_file(const char* filename, const uint8_t* data, size_t size);

} // namespace WebFileUtils

#endif
