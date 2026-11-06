#include "Frontend/Control/AppState.h"

#include <exception>
#include "Hardware/Core/Console.h"
#include "Hardware/Core/Logger.h"
#include "Frontend/Platform/PlatformUtils.h"
#include "Frontend/Control/RomCartridgeLoaderService.h"

namespace Control {

void AppState::ConnectBackendCallbacksAndConfig() {
    if (!backend) {
        return;
    }
    backend->SetOutputCallback(Console::OutputCallback);
    Console::SetInputResponseCallback([this](char c) {
        if (backend) {
            backend->InjectKey(c);
        }
    });
    backend->SetGPUEnabled(emulation.gpuEnabled);
    backend->SetTargetIPS(emulation.instructionsPerFrame);
    backend->SetCycleAccurate(emulation.cycleAccurate);
    backend->SetAutoReload(emulation.autoReload);
    backend->SetSDEnabled(emulation.sdEnabled);
    backend->SetESPEnabled(emulation.espEnabled);
    backend->SetRasterBeamSlowMotion(crt.rasterBeam);
    backend->GetSID().Init(cachedSidSampleRate);
}

bool AppState::CreateBackend(int sidSampleRate) {
    if (sidSampleRate > 0) {
        cachedSidSampleRate = sidSampleRate;
    }
    try {
        backend = Backend::CreateEmulatorBackend();
        if (!backend) {
            TriggerBackendError("Failed to create backend instance.", true);
            return false;
        }
        ConnectBackendCallbacksAndConfig();
        backendStatus = BackendStatus::Ready;
        lastBackendError.Clear();
        return true;
    } catch (const std::exception& e) {
        TriggerBackendError(e.what(), true);
        return false;
    } catch (...) {
        TriggerBackendError("Unknown error while creating backend instance.", true);
        return false;
    }
}

void AppState::DestroyBackend() {
    if (backend) {
        try {
            backend->Stop();
        } catch (...) {
        }
        backend.reset();
    }
    backendStatus = BackendStatus::Uninitialized;
}

bool AppState::RecreateBackend(int sidSampleRate, bool reloadRom) {
    DestroyBackend();
    bool created = CreateBackend(sidSampleRate > 0 ? sidSampleRate : cachedSidSampleRate);
    if (!created) {
        return false;
    }
    if (reloadRom && (!rom.bin.empty() || !rom.data.empty())) {
        bool isCartridge = rom.bin.ends_with(".65c") || rom.bin.ends_with(".c65");
        bool success = false;
        if (isCartridge) {
            success = Control::LoadCartridge(*this, rom.bin, rom.data);
        } else {
            success = Control::LoadBinaryRom(*this, rom.bin, rom.data);
        }
        if (success) {
            backend->Start();
        } else {
            return false;
        }
    } else {
        backend->Start();
    }
    backendStatus = BackendStatus::Ready;
    lastBackendError.Clear();
    return true;
}

void AppState::TriggerBackendError(std::string_view summary, bool fatal) {
    Core::Logger::Error("Backend Error: {}", summary);
    std::string trace = Frontend::Platform::CaptureStackTrace();
    lastBackendError.Set(summary, trace, fatal);
    backendStatus = BackendStatus::Error;
    if (fatal && backend) {
        try {
            backend->Stop();
        } catch (...) {
        }
    }
}

}  // namespace Control
