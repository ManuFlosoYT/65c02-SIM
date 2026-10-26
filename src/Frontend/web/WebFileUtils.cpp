#include "Frontend/web/WebFileUtils.h"
#include "Frontend/web/BrowserFileBridge.h"
#include "Frontend/web/SharedUrlLoader.h"

#ifdef TARGET_WASM
#include <emscripten.h>

// Global C-linkage functions for JS interface
extern "C" {

EMSCRIPTEN_KEEPALIVE
void Wasm_OnFilePicked(const char* filename, const uint8_t* data, int size) {
    if (WebFileUtils::onFilePickedCallback) {
        WebFileUtils::onFilePickedCallback(filename, data, size);
    }
}

EMSCRIPTEN_KEEPALIVE
void Wasm_LoadSharedData(const uint8_t* data, int size) {
    WebFileUtils::load_shared_payload(data, static_cast<size_t>(size));
}

EMSCRIPTEN_KEEPALIVE
void Wasm_CheckURLHash() {
    WebFileUtils::check_url_parameters();
}

} // extern "C"

#endif

namespace WebFileUtils {

void init_url_loader(Control::AppState* state) {
    Frontend::SharedUrlLoader::Init(state);
}

void check_url_parameters() {
    Frontend::SharedUrlLoader::CheckUrlParameters();
}

void show_toast(const char* message) {
    Frontend::BrowserFileBridge::ShowToast(message);
}

bool load_shared_payload(const uint8_t* data, size_t size) {
    return Frontend::SharedUrlLoader::LoadPayload(data, size);
}

void open_browser_file_picker(const char* accept) {
    Frontend::BrowserFileBridge::OpenFilePicker(accept);
}

void download_file(const char* filename, const uint8_t* data, size_t size) {
    Frontend::BrowserFileBridge::DownloadFile(filename, data, size);
}

void fetch_file(const char* url, const char* filename) {
    Frontend::BrowserFileBridge::FetchFile(url, filename);
}

} // namespace WebFileUtils
