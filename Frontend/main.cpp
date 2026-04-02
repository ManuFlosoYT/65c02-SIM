#include <ImGuiFileDialog.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#ifdef TARGET_WASM
#include <emscripten.h>
#endif


#include <fstream>
#include <iostream>
#include <span>
#include <string>

#include "Frontend/Control/AppState.h"
#include "Frontend/Control/Console.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Frontend/GUI/ConsoleWindow.h"
#include "Frontend/GUI/ControlWindow.h"
#include "Frontend/GUI/LCDWindow.h"
#include "Frontend/GUI/RegistersWindow.h"
#include "Frontend/GUI/ScriptConsoleWindow.h"
#include "Frontend/GUI/SIDViewerWindow.h"
#include "Frontend/GUI/Style/Style.h"
#include "Frontend/GUI/UpdatePopup.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "Frontend/GUI/IDEWindow.h"
#include "UpdateChecker.h"
#include "Frontend/MediaExporter.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif
#include "IconPixels.h"
#include <memory>
#ifndef TARGET_WASM
#include "Frontend/Compiler/CC65VFS.h"
#endif
#include "Hardware/Core/CartridgeLoader.h"

// Dedicated GPU
#if defined(__linux__) && !defined(TARGET_WASM)
#include <cstdlib>
#endif

using namespace Control;
using namespace Core;
using namespace Frontend;
using namespace Hardware;

// Dedicated GPU
#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static constexpr int sidSampleRate = 48000;

static bool InitializeSDL(SDL_Window*& window, SDL_GLContext& gl_context) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        std::cerr << "Error: " << SDL_GetError() << '\n';
        return false;
    }

    // GL 3.0 + GLSL 130
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                                          SDL_WINDOW_MAXIMIZED);
    window = SDL_CreateWindow("65C02 Simulator " PROJECT_VERSION, 1920, 1080, window_flags);
    if (window == nullptr) {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        return false;
    }

    // Set Window Icon
    SDL_Surface* icon = SDL_CreateSurfaceFrom(256, 256, SDL_PIXELFORMAT_RGBA32, (void*)icon_pixels_bin, 256 * 4);
    if (icon != nullptr) {
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    }

    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        std::cerr << "Failed to create GL context: " << SDL_GetError() << '\n';
        return false;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);  // Enable vsync

    // Initialize OpenGL loader
    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL loader (gladLoadGL)!\n";
        return false;
    }

    return true;
}

static void InitializeTextures(AppState& state) {
    glGenTextures(1, &state.render.vramTexture);
    glBindTexture(GL_TEXTURE_2D, state.render.vramTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    std::array<unsigned char, static_cast<std::size_t>(GPU::VRAM_HEIGHT) * GPU::VRAM_WIDTH * 3> emptyPixels{};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GPU::VRAM_WIDTH, GPU::VRAM_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 emptyPixels.data());

    glGenTextures(1, &state.render.profilerTexture);
    glBindTexture(GL_TEXTURE_2D, state.render.profilerTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &state.render.layoutTexture);
    glBindTexture(GL_TEXTURE_2D, state.render.layoutTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &state.render.sidTexture);
    glBindTexture(GL_TEXTURE_2D, state.render.sidTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glGenFramebuffers(1, &state.render.sidFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, state.render.sidFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.render.sidTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    state.crtFilter.Init(GPU::VRAM_WIDTH, GPU::VRAM_HEIGHT);
}

static void SetupStyle(){
    SetupImGuiStyle();
    SetupModernStyle();
    SetupFont();
}

static void InitializeImGui(SDL_Window* window, SDL_GLContext gl_context, const char* glsl_version) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& imgui_io = ImGui::GetIO();
    (void)imgui_io;
    imgui_io.IniFilename = nullptr;

    float dpi_scale = SDL_GetWindowDisplayScale(window);
    if (dpi_scale <= 0.0F) {
        dpi_scale = 1.0F;
    }

    imgui_io.FontGlobalScale = 1.5F / dpi_scale;

    ImGui::StyleColorsDark();
    SetupStyle();
    ImGui::GetStyle().ScaleAllSizes(1.0F / dpi_scale);
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

static void HandleSDLEvents(bool& done, SDL_Window* window) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            done = true;
        }
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) {
            done = true;
        }
    }
}

static void HandleROMFilePicker(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.Pause();
            std::string errorMsg;
            if (state.emulator.Init(filePathName, errorMsg)) {
                Console::Clear();
                state.rom.bin = filePathName;
                state.rom.loaded = true;
                state.rom.symbols.Clear();
                state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
                state.emulator.ClearProfiler();
            } else {
                ImGui::OpenPopup("ErrorLoadingROM");
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleCartridgeFilePicker(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("ChooseCartridgeDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.Pause();
            std::string errorMsg;
            Core::Cartridge cart;
            if (Core::CartridgeLoader::Load(filePathName, cart, errorMsg)) {
                Control::ApplyCartridgeConfig(state, cart);
                ImGui::OpenPopup("Cartridge Loaded");
                if (state.emulator.InitFromMemory(cart.romData.data(), cart.romData.size(), cart.romFileName, errorMsg)) {
                    state.rom.bin = filePathName;
                    state.rom.loaded = true;
                    state.rom.symbols.Clear();
                    state.emulator.ClearProfiler();
                } else {
                    ImGui::OpenPopup("ErrorLoadingROM");
                }
            } else {
                std::cerr << "Cartridge load error: " << errorMsg << "\n";
                ImGui::OpenPopup("ErrorLoadingROM");
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleSaveStateFilePicker(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("SaveStateDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.Pause();
            if (!state.emulator.SaveState(filePathName)) {
                ImGui::OpenPopup("ErrorSavingState");
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleLoadStateFilePicker(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("LoadStateDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.Pause();
            state.emulator.LoadState(filePathName, state.emulation.forceLoadSaveState);
            auto loadResult = state.emulator.GetLastLoadResult();
            bool loadedOk = loadResult == SavestateLoadResult::Success ||
                            loadResult == SavestateLoadResult::VersionMismatch ||
                            loadResult == SavestateLoadResult::HashMismatch;

            if (loadedOk) {
                state.rom.loaded = true;
                state.emulation.gpuEnabled = state.emulator.IsGPUEnabled();
                state.emulation.instructionsPerFrame = state.emulator.GetTargetIPS();
                state.emulation.cycleAccurate = state.emulator.IsCycleAccurate();
                state.emulation.autoReload = state.emulator.IsAutoReloadEnabled();
                state.rom.bin = state.emulator.GetCurrentBinPath();
            }

            if (loadResult != SavestateLoadResult::Success) {
                ImGui::OpenPopup("SavestateFeedback");
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleDialogs(AppState& state) {
#ifndef TARGET_WASM
    HandleROMFilePicker(state);
    HandleCartridgeFilePicker(state);
    HandleSaveStateFilePicker(state);
    HandleLoadStateFilePicker(state);
#endif
}

static void DrawROMAndStatePopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("ErrorLoadingROM", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Error loading ROM. Please check the file.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("ErrorSavingState", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Error saving state. Please check your permissions.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SavestateFeedback", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        auto result = state.emulator.GetLastLoadResult();
        if (result == SavestateLoadResult::VersionMismatch || result == SavestateLoadResult::HashMismatch) {
            ImGui::TextColored(ImVec4(1.0F, 1.0F, 0.0F, 1.0F), "Warning: Savestate compatibility issue");  // NOLINT
            if (result == SavestateLoadResult::VersionMismatch) {
                ImGui::TextUnformatted("Version mismatch detected:");
                ImGui::BulletText("Saved: %s", state.emulator.GetLastLoadVersion().c_str());  // NOLINT
                ImGui::BulletText("Current: %s", PROJECT_VERSION);                            // NOLINT
            } else {
                ImGui::TextUnformatted(
                    "Hash mismatch. The data might be modified or "
                    "corrupt.");
            }
            ImGui::TextUnformatted(
                "The state was loaded, but some things might not work "
                "correctly.");
        } else if (result == SavestateLoadResult::StructuralError) {
            ImGui::TextColored(ImVec4(1.0F, 0.0F, 0.0F, 1.0F), "Error: Failed to load state");  // NOLINT
            ImGui::TextUnformatted("The data is structurally incompatible or corrupted.");
        } else {
            ImGui::TextUnformatted("An unknown error occurred while loading the state.");
        }

        ImGui::Spacing();
        ImGui::SetNextWindowSize(ImVec2(120, 0));
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void DrawSDCardPopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    if (state.popups.sdCardDisabled) {
        ImGui::OpenPopup("SD Card Disabled");
        state.popups.sdCardDisabled = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Card Disabled", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("The SD Card device is currently disabled in the Memory Layout.");
        ImGui::TextUnformatted("Please enable it in the Memory Layout (Debugger -> Memory Layout) to use it.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.popups.sdCardWebWarning) {
        ImGui::OpenPopup("SD Card Web Warning");
        state.popups.sdCardWebWarning = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Card Web Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("SD Card Mounted (Web Port)");
        ImGui::Separator();
        ImGui::TextUnformatted("Due to browser security limitations, changes made to the SD card");
        ImGui::TextUnformatted("are stored in a virtual filesystem and NOT synced to your local file.");
        ImGui::Spacing();
        ImGui::TextUnformatted("To save your changes, you MUST use 'Unmount & Save (Download)'");
        ImGui::TextUnformatted("or the 'Save Changes' button in the Control Window.");
        ImGui::Spacing();
        if (ImGui::Button("I Understand", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void DrawCartridgePopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));

    if (ImGui::BeginPopupModal("Cartridge Loaded", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        const auto& cart = state.emulator.GetCartridge();
        if (cart.loaded) {
            std::string nameLine = "Name: " + cart.metadata.name;
            std::string authorLine = "Author: " + cart.metadata.author;
            std::string versionLine = "Version: " + cart.metadata.version;
            ImGui::TextUnformatted(nameLine.c_str());
            ImGui::TextUnformatted(authorLine.c_str());
            ImGui::TextUnformatted(versionLine.c_str());
            ImGui::Separator();
            ImGui::TextUnformatted(cart.metadata.description.c_str());
        }
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void DrawPopups(AppState& state) {
    DrawROMAndStatePopups(state);
    DrawSDCardPopups(state);
    DrawCartridgePopups(state);
}

static void DrawGUIWindows(AppState& state, const ImVec2& work_pos, const ImVec2& work_size, float top_section_height, ImGuiWindowFlags windowFlags) {
    GUI::DrawUpdatePopup(state);
    GUI::DrawControlWindow(state, work_pos, work_size, top_section_height, windowFlags);
    GUI::DrawLCDWindow(state, work_pos, work_size, top_section_height, windowFlags);
    GUI::DrawSIDViewerWindow(state, work_pos, work_size, top_section_height, windowFlags);
    GUI::DrawRegistersWindow(state, work_pos, work_size, top_section_height, windowFlags);
    GUI::DrawConsoleWindow(state, work_pos, work_size, top_section_height, windowFlags);
    GUI::DrawScriptConsoleWindow(state, work_pos, work_size, top_section_height, windowFlags);
    state.crt.time = static_cast<float>(SDL_GetTicks()) / 1000.0F;
    GUI::DrawVRAMViewerWindow(state, work_pos, work_size, top_section_height, windowFlags);
    GUI::DrawIDEWindow(state);
}

static void HandleSIDWindowCapture(AppState& state) {
    const ImGuiIO& imguiIO = ImGui::GetIO();
    int screenHeightPixels = static_cast<int>(imguiIO.DisplaySize.y * imguiIO.DisplayFramebufferScale.y);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); // Read from screen backbuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state.render.sidFBO);

    float scaleX = imguiIO.DisplayFramebufferScale.x;
    float scaleY = imguiIO.DisplayFramebufferScale.y;

    float windowX = state.render.sidWindowPos[0];
    float windowY = state.render.sidWindowPos[1];
    float windowW = state.render.sidWindowSize[0];
    float windowH = state.render.sidWindowSize[1];

    if (windowW <= 0 || windowH <= 0) {
        return;
    }

    // OpenGL coordinates start from bottom-left (pixels)
    int srcX0 = static_cast<int>(windowX * scaleX);
    int srcY0 = screenHeightPixels - static_cast<int>((windowY + windowH) * scaleY);
    int srcX1 = static_cast<int>((windowX + windowW) * scaleX);
    int srcY1 = screenHeightPixels - static_cast<int>(windowY * scaleY);

    // Calculate centered aspect ratio in 720p
    float targetW = 1280.0F;
    float targetH = 720.0F;
    float winAR = windowW / windowH;
    float targetAR = targetW / targetH;

    int drawW = 0;
    int drawH = 0;
    int offsetX = 0;
    int offsetY = 0;

    if (winAR > targetAR) {
        drawW = 1280;
        drawH = static_cast<int>(1280.0F / winAR);
        offsetY = (720 - drawH) / 2;
    } else {
        drawH = 720;
        drawW = static_cast<int>(720.0F * winAR);
        offsetX = (1280 - drawW) / 2;
    }

    // Clear FBO each frame to avoid artifacts in pillarbox areas
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    // Blit and flip vertically for FFmpeg compatibility
    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
                        offsetX, offsetY + drawH, offsetX + drawW, offsetY, // Centered and Inverted Y
                        GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static bool InitializeExporter(AppState& state, std::unique_ptr<MediaExporter>& mediaExporter) {
    auto& sid = state.emulator.GetSID();

    mediaExporter = std::make_unique<MediaExporter>();
    const MediaExporter::AudioParams audioParams{.sampleRate = sidSampleRate, .channels = 1, .bitDepth = 16};

    int videoW = 1280;
    int videoH = 720;
    int procW = 1280;
    int procH = 720;

    if (state.emulation.recordingSettings.type != RecordingType::SIDWindow) {
        videoW = GPU::VRAM_WIDTH;
        videoH = GPU::VRAM_HEIGHT;
        procW = state.render.lastDisplayW > 0 ? state.render.lastDisplayW : 600;
        procH = state.render.lastDisplayH > 0 ? state.render.lastDisplayH : 450;
    }

    bool initOk = mediaExporter->Initialize(state.emulation.recordingVideoPath, videoW, videoH,
                                             procW, procH, audioParams,
                                             state.emulation.recordingSettings.type,
                                             state.emulation.recordingSettings.format,
                                             state.emulation.recordingSettings.audioFormat, // Pass audioFormat
                                             state.emulation.recordingSettings.recordRaw,
                                             state.emulation.recordingSettings.recordProcessed);
    if (!initOk) {
        state.emulation.isRecordingVideo = false;
        mediaExporter.reset();
        return false;
    }

    sid.SetAudioCallback([mPtr = mediaExporter.get()](const int16_t* samples, int count) {
        std::vector<float> floatSamples(count);
        std::span<const int16_t> samplesSpan(samples, static_cast<std::size_t>(count));
        for (int idx = 0; idx < count; ++idx) {
            floatSamples[idx] = static_cast<float>(samplesSpan[idx]) / 32767.0F;
        }
        mPtr->PushAudio(floatSamples.data(), count);
    });
    return true;
}

static void HandleAutoStop(AppState& state) {
    // Auto-stop recording if emulation has halted or stopped after having run
    if (!state.emulation.isRecordingVideo) {
        return;
    }

    bool shouldAutoStop = (state.emulator.IsHalted() || !state.emulator.IsRunning());
    if (shouldAutoStop && state.emulator.GetTotalCycles() > 0) {
        state.emulation.isRecordingVideo = false;
        if (state.emulator.GetSID().IsRecording()) {
            state.emulator.GetSID().StopRecording();
        }
    }
}

static void UpdateMediaRecording(AppState& state, std::unique_ptr<MediaExporter>& mediaExporter) {
    HandleAutoStop(state);

    if (state.emulation.isRecordingVideo && !mediaExporter) {
        InitializeExporter(state, mediaExporter);
    } else if (!state.emulation.isRecordingVideo && mediaExporter) {
        state.emulator.GetSID().ClearAudioCallback();
        mediaExporter->Finalize();
        mediaExporter.reset();
    }

    if (mediaExporter) {
        uint32_t processedTex = state.render.lastDisplayTex != 0 ? state.render.lastDisplayTex : state.render.vramTexture;
        uint32_t rawTex = state.render.vramTexture;

        if (state.emulation.recordingSettings.type == RecordingType::SIDWindow) {
            HandleSIDWindowCapture(state);
            rawTex = state.render.sidTexture;
            processedTex = state.render.sidTexture;
        }

        mediaExporter->PushFrames(rawTex, processedTex, state.emulator.IsPaused());
    }
}

static void Cleanup(AppState& state, SDL_Window* window, SDL_GLContext gl_context) {
    if (state.emulator.GetCartridge().loaded && !state.emulator.GetCartridge().sdCardPath.empty()) {
        Core::CartridgeLoader::SaveSDToZip(state.emulator.GetCartridge());
    }
    state.emulator.Stop();
    state.emulator.GetSID().Close();
    state.crtFilter.Destroy();
    glDeleteTextures(1, &state.render.vramTexture);
    glDeleteTextures(1, &state.render.profilerTexture);
    glDeleteTextures(1, &state.render.layoutTexture);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
#ifndef TARGET_WASM
    CC65VFS::Cleanup();
#endif
}

struct Args {
    bool headless = false;
    std::string scriptPath;
    std::string romPath;
    std::string dumpMemPath;
    int runCycles = 0;
};

static Args ParseArgs(std::span<const char* const> argv) {
    Args args;
    for (std::size_t i = 1; i < argv.size(); ++i) {
        std::string_view arg = argv[i];
        if (arg == "--headless") {
            args.headless = true;
        } else if (arg == "--script" && i + 1 < argv.size()) {
            args.scriptPath = argv[++i];
        } else if (arg == "--run-cycles" && i + 1 < argv.size()) {
            args.runCycles = std::stoi(argv[++i]);
        } else if (arg == "--dump-mem" && i + 1 < argv.size()) {
            args.dumpMemPath = argv[++i];
        } else if (arg == "--rom" && i + 1 < argv.size()) {
            args.romPath = argv[++i];
        } else if (args.romPath.empty() && arg[0] != '-') {
            args.romPath = std::string(arg);
        }
    }
    return args;
}

static int RunHeadless(const Args& args) {
    Core::Emulator emulator;
    emulator.GetSID().Init(sidSampleRate);
    emulator.SetGPUEnabled(false);

    if (!args.romPath.empty()) {
        std::string errorMsg;
        if (!emulator.Init(args.romPath, errorMsg)) {
            std::cerr << "Headless: Failed to load ROM: " << errorMsg << '\n';
            return -1;
        }
    }

    if (!args.scriptPath.empty()) {
        emulator.GetScriptEngine().SetMirrorToStdout(true);
        emulator.GetScriptEngine().LoadAndRun(args.scriptPath);
    }

    if (args.runCycles > 0) {
        emulator.Resume();
        for (int i = 0; i < args.runCycles; ++i) {
            emulator.Step();
        }
        emulator.Pause();
    }

    if (!args.dumpMemPath.empty()) {
        std::ofstream out(args.dumpMemPath, std::ios::binary);
        if (out.is_open()) {
            auto& mem = emulator.GetMem();
            for (uint32_t addr = 0; addr <= 0xFFFF; ++addr) {
                char val = static_cast<char>(mem.Read<true>(static_cast<uint16_t>(addr)));
                out.write(&val, 1);
            }
        } else {
            std::cerr << "Headless: Failed to open dump file " << args.dumpMemPath << '\n';
        }
    }

    emulator.Stop();
    emulator.GetSID().Close();
    return 0;
}

// --- Emscripten Loop Support ---

struct MainLoopArgs {
    SDL_Window* window;
    SDL_GLContext gl_context;
    AppState* state;
    std::unique_ptr<MediaExporter>* mediaExporter;
    bool* done;
};

static void MainLoop(void* arg) {
    auto* args = static_cast<MainLoopArgs*>(arg);
    auto& state = *args->state;
    auto& done = *args->done;
    auto* window = args->window;

    HandleSDLEvents(done, window);

    if (!state.emulator.IsPaused()) {
        state.emulation.instructionsPerFrame = state.emulator.GetTargetIPS();
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Layout Configuration
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    float top_section_height = work_size.y * 0.27F;

    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

    HandleDialogs(state);
    DrawPopups(state);

    // Draw all windows
    DrawGUIWindows(state, work_pos, work_size, top_section_height, windowFlags);

    ImGui::Render();
    ImGuiIO& imgui_io = ImGui::GetIO();
    glViewport(0, 0, (int)imgui_io.DisplaySize.x, (int)imgui_io.DisplaySize.y);
    glClearColor(0.0F, 0.0F, 0.0F, 1.00F);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifndef TARGET_WASM
    UpdateMediaRecording(state, *args->mediaExporter);
#endif

    SDL_GL_SwapWindow(window);

#ifdef TARGET_WASM
    if (done) {
        emscripten_cancel_main_loop();
        Cleanup(state, window, args->gl_context);
    }
#endif
}

int main(int argc, char* argv[]) {
#if defined(__linux__) && !defined(TARGET_WASM)
    setenv("DRI_PRIME", "1", 0);                        // Mesa (AMD/Intel)
    setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 0);        // NVIDIA Propietary
    setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 0);   // NVIDIA Propietary
#endif
    const Args args = ParseArgs(std::span<const char* const>(argv, static_cast<std::size_t>(argc)));

#ifndef TARGET_WASM
    CC65VFS::Initialize();
#endif

    if (args.headless) {
        int ret = RunHeadless(args);
#ifndef TARGET_WASM
        CC65VFS::Cleanup();
#endif
        return ret;
    }

    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;
    if (!InitializeSDL(window, gl_context)) {
        return -1;
    }

    static AppState state;
    state.emulator.GetSID().Init(sidSampleRate);

#ifndef TARGET_WASM
    // Check for updates
    UpdateChecker::CheckForUpdates(PROJECT_VERSION, [](bool available, const std::string& version) {
        if (available) {
            state.update.latestVersionTag = version;
            state.update.available = true;
        }
    });
#endif

    InitializeTextures(state);

#ifdef TARGET_WASM
    const char* glsl_version = "#version 100"; // WebGL 1/2 compatible
#else
    const char* glsl_version = "#version 130";
#endif
    InitializeImGui(window, gl_context, glsl_version);

    if (!args.romPath.empty()) {
        state.rom.bin = args.romPath;
        std::string errorMsg;
        if (state.emulator.Init(state.rom.bin, errorMsg)) {
            state.rom.loaded = true;
            state.rom.symbols.Clear();
            state.emulator.ClearProfiler();
        } else {
            std::cerr << "Failed to load ROM from args: " << errorMsg << '\n';
        }
    }
    state.emulator.SetOutputCallback(Console::OutputCallback);
    state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);

    static bool done = false;
    state.emulator.Start();
    
#ifndef TARGET_WASM
    static std::unique_ptr<MediaExporter> mediaExporter = nullptr;
#endif

#ifdef TARGET_WASM
    static MainLoopArgs loopArgs = { .window = window, .gl_context = gl_context, .state = &state, .mediaExporter = nullptr, .done = &done };
    emscripten_set_main_loop_arg(MainLoop, &loopArgs, 0, 1);
#else
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    Uint64 frameStart = 0;
    int frameTime = 0;

    MainLoopArgs loopArgs = { .window = window, .gl_context = gl_context, .state = &state, .mediaExporter = &mediaExporter, .done = &done };

    while (!done) {
        frameStart = SDL_GetTicks();
        MainLoop(&loopArgs);
        frameTime = (int)(SDL_GetTicks() - frameStart);
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    Cleanup(state, window, gl_context);
#endif

    return 0;
}

