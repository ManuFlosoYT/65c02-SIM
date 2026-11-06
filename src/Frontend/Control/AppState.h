#pragma once

#include <glad/gl.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "Frontend/Control/EmulationState.h"
#include "Frontend/Control/GLRenderTargets.h"
#include "Frontend/Control/IDEState.h"
#include "Frontend/Control/UIModalState.h"
#include "Frontend/GUI/Video/CRTFilter.h"
#include "Backend/IEmulatorBackend.h"
#include "Backend/BackendFactory.h"
#include "Backend/BackendError.h"
#include "Hardware/Core/MemoryScanner.h"
#include "Hardware/Core/SymbolTable.h"

namespace Control {

using namespace Core;
using namespace Hardware;

struct CRTSettings {
    bool scanlines = true;
    bool interlacing = true;
    bool curvature = true;
    bool chromatic = true;
    bool blur = true;
    bool shadowMask = true;
    bool vignette = true;
    bool cornerRounding = true;
    bool glassGlare = true;
    bool colorBleeding = true;
    bool noise = true;
    bool vsyncJitter = true;
    bool phosphorDecay = true;
    bool bloom = true;
    bool ghosting = true;
    bool halation = true;
    bool moire = true;
    bool rasterBeam = false;
    bool sharpPixels = true;
    bool humBar = true;
    GUI::CRTMonitorType monitorType = GUI::CRTMonitorType::RGB;
    float gamma = 2.75F;
    float time = 0.0F;

    [[nodiscard]] constexpr bool HasActiveFilters() const noexcept {
        return scanlines || interlacing || curvature || chromatic || blur ||
               shadowMask || vignette || cornerRounding || glassGlare ||
               colorBleeding || noise || vsyncJitter || phosphorDecay ||
               bloom || ghosting || halation || moire || humBar ||
               monitorType != GUI::CRTMonitorType::RGB;
    }
};

struct AppState {
    std::unique_ptr<Backend::IEmulatorBackend> backend;
    enum class BackendStatus { Uninitialized, Initializing, Ready, Error };
    BackendStatus backendStatus = BackendStatus::Uninitialized;
    Backend::BackendErrorState lastBackendError;
    int cachedSidSampleRate = 48000;

    ROMState rom;
    ScriptState script;
    EmulationState emulation;
    DebuggerState debugger;
    UpdateState update;
    CRTSettings crt;
    RenderState render;
    PopupState popups;
    SDKState sdk;
    IDEState ide;
    NetworkSnifferState sniffer;
    ScannerState scanner;
    bool requestExit = false;

    GUI::CRTFilter crtFilter;

    [[nodiscard]] bool HasBackend() const noexcept {
        return backend != nullptr && backendStatus == BackendStatus::Ready;
    }

    bool CreateBackend(int sidSampleRate = 48000);
    void DestroyBackend();
    bool RecreateBackend(int sidSampleRate = 48000, bool reloadRom = true);
    void TriggerBackendError(std::string_view summary, bool fatal = true);
    void ConnectBackendCallbacksAndConfig();
};

inline void SyncUIStateFromEmulator(AppState& state) {
    if (!state.backend) {
        return;
    }
    state.rom.loaded = true;
    state.emulation.gpuEnabled = state.backend->IsGPUEnabled();
    state.emulation.instructionsPerFrame = state.backend->GetTargetIPS();
    state.emulation.cycleAccurate = state.backend->IsCycleAccurate();
    state.emulation.autoReload = state.backend->IsAutoReloadEnabled();
    state.emulation.sdLatencyEnabled = state.backend->GetSDCard().IsReadLatencyEnabled();
    state.rom.bin = state.backend->GetCurrentBinPath();
}

}  // namespace Control
