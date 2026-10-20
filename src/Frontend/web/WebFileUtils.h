#pragma once

#ifdef TARGET_WASM
#include <emscripten.h>
#include <emscripten/em_js.h>
#include <string>
#include <vector>
#include <functional>

namespace Control {
struct AppState;
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void Wasm_OnFilePicked(const char* filename, const uint8_t* data, int size);

    EMSCRIPTEN_KEEPALIVE
    void Wasm_LoadSharedData(const uint8_t* data, int size);

    EMSCRIPTEN_KEEPALIVE
    void Wasm_CheckURLHash();
}

namespace WebFileUtils {

// Global callbacks to be called from JS
inline std::function<void(const char*, const uint8_t*, int)> onFilePickedCallback;

void init_url_loader(Control::AppState* state);
void check_url_parameters();
bool load_shared_payload(const uint8_t* data, size_t size);
void show_toast(const char* message);

void open_browser_file_picker(const char* accept);
void download_file(const char* filename, const uint8_t* data, size_t size);
void fetch_file(const char* url, const char* filename);

} // namespace WebFileUtils

#endif
