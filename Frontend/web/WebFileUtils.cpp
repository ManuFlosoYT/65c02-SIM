#include "Frontend/web/WebFileUtils.h"

#ifdef TARGET_WASM
#include <emscripten.h>
#include <emscripten/em_js.h>
#include <vector>

// Global C-linkage functions for JS interface
extern "C" {

EMSCRIPTEN_KEEPALIVE
void Wasm_OnFilePicked(const char* filename, const uint8_t* data, int size) {
    if (WebFileUtils::onFilePickedCallback) {
        WebFileUtils::onFilePickedCallback(filename, data, size);
    }
}

EM_JS(void, impl_open_browser_file_picker, (const char* accept), {
    var input = document.createElement('input');
    input.type = 'file';
    input.accept = UTF8ToString(accept);
    input.onchange = function(e) {
        var file = e.target.files[0];
        if (!file) return;
        var reader = new FileReader();
        reader.onload = function() {
            var data = new Uint8Array(reader.result);
            if (!Module._malloc) {
                console.error("Module._malloc is not defined!");
                return;
            }
            var buffer = Module._malloc(data.length);
            var heap = Module['HEAPU8'] || Module.HEAPU8;
            if (!heap) {
                console.error("Emscripten HEAPU8 not found! Module properties:", Object.keys(Module));
                Module._free(buffer);
                return;
            }
            heap.set(data, buffer);
            var filenameBytes = lengthBytesUTF8(file.name) + 1;
            var filenamePtr = Module._malloc(filenameBytes);
            stringToUTF8(file.name, filenamePtr, filenameBytes);
            
            // Using Module.ccall for safer argument passing and signature handling
            Module['ccall']('Wasm_OnFilePicked', 
                null, 
                ['number', 'number', 'number'], 
                [filenamePtr, buffer, data.length]
            );
            
            Module._free(filenamePtr);
            Module._free(buffer);
        };
        reader.readAsArrayBuffer(file);
    };
    input.click();
});

EM_JS(void, impl_download_file, (const char* filename, const uint8_t* data, size_t size), {
    var rawData = Module.HEAPU8.subarray(data, data + size);
    var nonSharedData = new Uint8Array(size);
    nonSharedData.set(rawData);
    var blob = new Blob([nonSharedData], {type: 'application/octet-stream'});
    var url = window.URL.createObjectURL(blob);
    var a = document.createElement('a');
    a.href = url;
    a.download = Module['UTF8ToString'](filename);
    document.body.appendChild(a);
    a.click();
    window.URL.revokeObjectURL(url);
    document.body.removeChild(a);
});

EM_JS(void, impl_fetch_file, (const char* url, const char* filename), {
    const urlStr = UTF8ToString(url);
    const fileStr = UTF8ToString(filename);
    fetch(urlStr)
        .then(response => {
            if (!response.ok) throw new Error('Network response was not ok');
            return response.arrayBuffer();
        })
        .then(buffer => {
            const data = new Uint8Array(buffer);
            const dataPtr = Module._malloc(data.length);
            Module.HEAPU8.set(data, dataPtr);
            
            const filenameBytes = lengthBytesUTF8(fileStr) + 1;
            const filenamePtr = Module._malloc(filenameBytes);
            stringToUTF8(fileStr, filenamePtr, filenameBytes);
            
            Module['ccall']('Wasm_OnFilePicked', 
                null, 
                ['number', 'number', 'number'], 
                [filenamePtr, dataPtr, data.length]
            );
            
            Module._free(filenamePtr);
            Module._free(dataPtr);
        })
        .catch(err => console.error("Fetch failed:", err));
});

} // extern "C"

namespace WebFileUtils {

void open_browser_file_picker(const char* accept) {
    impl_open_browser_file_picker(accept);
}

void download_file(const char* filename, const uint8_t* data, size_t size) {
    impl_download_file(filename, data, size);
}

void fetch_file(const char* url, const char* filename) {
    impl_fetch_file(url, filename);
}

} // namespace WebFileUtils

#endif
