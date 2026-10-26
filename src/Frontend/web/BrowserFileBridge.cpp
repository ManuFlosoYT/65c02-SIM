#include "Frontend/web/BrowserFileBridge.h"

#ifdef TARGET_WASM
#include <emscripten.h>
#include <emscripten/em_js.h>

extern "C" {

EM_JS(void, impl_show_toast, (const char* msg), {
    var text = UTF8ToString(msg);
    if (typeof window !== 'undefined' && window.showToast) {
        window.showToast(text);
    }
});

EM_JS(void, impl_check_url_hash, (), {
    if (typeof window === 'undefined') return;
    var hash = window.location.hash;
    var payload = null;
    if (hash && hash.length > 1) {
        if (hash.startsWith('#')) hash = hash.substring(1);
        var params = new URLSearchParams(hash);
        payload = params.get('code') || params.get('cart') || params.get('state') || params.get('demo');
        if (!payload && hash.indexOf('=') === -1 && hash.length > 10) {
            payload = hash;
        }
    }
    if (!payload && window.location.search) {
        var searchParams = new URLSearchParams(window.location.search);
        payload = searchParams.get('code') || searchParams.get('cart') || searchParams.get('state') || searchParams.get('demo');
    }
    if (!payload || payload.length === 0) return;
    if (window._lastLoadedHashPayload === payload) return;
    window._lastLoadedHashPayload = payload;

    try {
        var b64 = payload.trim().replace(/-/g, '+').replace(/_/g, '/');
        while (b64.length % 4) b64 += '=';
        var binary = atob(b64);
        var len = binary.length;
        var bytes = new Uint8Array(len);
        for (var i = 0; i < len; i++) {
            bytes[i] = binary.charCodeAt(i);
        }
        var buf = Module._malloc(len);
        Module.HEAPU8.set(bytes, buf);
        Module['ccall']('Wasm_LoadSharedData', null, ['number', 'number'], [buf, len]);
        Module._free(buf);
    } catch (e) {
        console.error("Failed to load demo from URL:", e);
    }
});

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

#endif

namespace Frontend {

void BrowserFileBridge::OpenFilePicker(const char* accept) {
#ifdef TARGET_WASM
    impl_open_browser_file_picker(accept);
#else
    (void)accept;
#endif
}

void BrowserFileBridge::DownloadFile(const char* filename, const uint8_t* data, size_t size) {
#ifdef TARGET_WASM
    impl_download_file(filename, data, size);
#else
    (void)filename;
    (void)data;
    (void)size;
#endif
}

void BrowserFileBridge::FetchFile(const char* url, const char* filename) {
#ifdef TARGET_WASM
    impl_fetch_file(url, filename);
#else
    (void)url;
    (void)filename;
#endif
}

void BrowserFileBridge::ShowToast(const char* message) {
#ifdef TARGET_WASM
    impl_show_toast(message);
#else
    (void)message;
#endif
}

void BrowserFileBridge::CheckUrlHash() {
#ifdef TARGET_WASM
    impl_check_url_hash();
#endif
}

}  // namespace Frontend
