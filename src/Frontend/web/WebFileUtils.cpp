#include "Frontend/web/WebFileUtils.h"

#ifdef TARGET_WASM
#include <emscripten.h>
#include <emscripten/em_js.h>
#include <vector>
#include <span>
#include <cstring>
#include <cstdio>
#include <miniz.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/Control/Console.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/CartridgeLoader.h"

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

namespace WebFileUtils {

static Control::AppState* g_appState = nullptr;

void init_url_loader(Control::AppState* state) {
    g_appState = state;
    check_url_parameters();
}

void check_url_parameters() {
    impl_check_url_hash();
}

void show_toast(const char* message) {
    impl_show_toast(message);
}

bool load_shared_payload(const uint8_t* data, size_t size) {
    if (g_appState == nullptr || data == nullptr || size == 0) {
        return false;
    }
    auto& state = *g_appState;

    std::vector<uint8_t> uncompressed;
    const uint8_t* payloadData = data;
    size_t payloadSize = size;

    if (size > 4) {
        uint32_t origSize = static_cast<uint32_t>(data[0]) |
                            (static_cast<uint32_t>(data[1]) << 8) |
                            (static_cast<uint32_t>(data[2]) << 16) |
                            (static_cast<uint32_t>(data[3]) << 24);
        if (origSize > 0 && origSize <= 8 * 1024 * 1024) {
            uncompressed.resize(origSize);
            mz_ulong destLen = origSize;
            if (mz_uncompress(uncompressed.data(), &destLen, data + 4, static_cast<mz_ulong>(size - 4)) == MZ_OK &&
                destLen == origSize) {
                payloadData = uncompressed.data();
                payloadSize = uncompressed.size();
            }
        }
    }

    if (payloadSize >= 4 && payloadData[0] == 'P' && payloadData[1] == 'K' &&
        payloadData[2] == 0x03 && payloadData[3] == 0x04) {
        state.emulator.Pause();
        std::string errorMsg;
        Core::Cartridge cart;
        if (Core::CartridgeLoader::LoadFromMemory(payloadData, payloadSize, cart, errorMsg)) {
            Control::ApplyCartridgeConfig(state, cart);
            if (state.emulator.InitFromMemory(cart.romData, cart.romFileName, errorMsg)) {
                state.rom.bin = cart.metadata.name.empty() ? "shared_demo.65c" : cart.metadata.name;
                state.rom.loaded = true;
                state.rom.symbols.Clear();
                state.emulator.ClearProfiler();
                state.emulator.Resume();
                Console::Clear();
                std::cout << "Loaded shared cartridge: " << state.rom.bin << "\n";
                show_toast("Loaded demo from URL!");
                return true;
            }
        }
    }

    if (payloadSize >= 11 && memcmp(payloadData, "SIM65C02SST", 11) == 0) {
        std::string virtualPath = "/tmp/shared_demo.savestate";
        FILE* f = fopen(virtualPath.c_str(), "wb");
        if (f != nullptr) {
            fwrite(payloadData, 1, payloadSize, f);
            fclose(f);
            state.emulator.Pause();
            if (state.emulator.LoadState(virtualPath, true)) {
                state.rom.loaded = true;
                state.emulation.gpuEnabled = state.emulator.IsGPUEnabled();
                state.emulation.instructionsPerFrame = state.emulator.GetTargetIPS();
                state.emulation.cycleAccurate = state.emulator.IsCycleAccurate();
                state.emulation.autoReload = state.emulator.IsAutoReloadEnabled();
                state.emulation.sdLatencyEnabled = state.emulator.GetSDCard().IsReadLatencyEnabled();
                state.rom.bin = state.emulator.GetCurrentBinPath();
                state.emulator.Resume();
                std::cout << "Loaded shared savestate.\n";
                show_toast("Loaded state from URL!");
                return true;
            }
        }
    }

    std::span<const uint8_t> dataSpan(payloadData, payloadSize);
    state.rom.data.assign(dataSpan.begin(), dataSpan.end());
    state.emulator.Pause();
    std::string errorMsg;
    if (state.emulator.InitFromMemory(state.rom.data, "shared_demo.bin", errorMsg)) {
        Console::Clear();
        state.rom.bin = "shared_demo.bin";
        state.rom.loaded = true;
        state.rom.symbols.Clear();
        state.emulator.ClearProfiler();
        state.emulator.ClearCartridge();
        state.emulator.Resume();
        std::cout << "Loaded shared ROM.\n";
        show_toast("Loaded ROM from URL!");
        return true;
    }

    return false;
}

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
