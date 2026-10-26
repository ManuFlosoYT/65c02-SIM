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
#include <iomanip>
#include <iostream>
#include <charconv>
#include <span>
#include <string>

#include "Frontend/Control/AppState.h"
#include "Frontend/CliArgsParser.h"
#include "Frontend/HeadlessRunner.h"
#include "Hardware/Core/Console.h"
#include "Frontend/GUI/Style/Style.h"
#include "UpdateChecker.h"
#include "Frontend/MediaExporter.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif
#include "IconPixels.h"
#include <memory>
#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerVFS.h"
#include "Frontend/Compiler/CompilerFrontend.h"
#include "Hardware/Core/SymbolTable.h"
#endif
#include "Hardware/Core/CartridgeLoader.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Frontend/GUI/UIModules.h"
#include "Frontend/Control/SDKManager.h"
#include "Frontend/GUI/MIDI/MIDIWorkspaceWindow.h"
#if defined(__linux__) && !defined(TARGET_WASM)
#include <malloc.h>
#endif

using namespace Control;
using namespace Core;
using namespace Frontend;
using namespace Hardware;

static constexpr int sidSampleRate = 48000;

static bool TryInitializeSDL(SDL_Window*& window, SDL_GLContext& gl_context) {
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
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

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
        SDL_DestroyWindow(window);
        window = nullptr;
        return false;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);  // Enable vsync

    // Initialize OpenGL loader
    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL loader (gladLoadGL)!\n";
        SDL_GL_DestroyContext(gl_context);
        gl_context = nullptr;
        SDL_DestroyWindow(window);
        window = nullptr;
        return false;
    }

    return true;
}

static bool InitializeSDL(SDL_Window*& window, SDL_GLContext& gl_context) {
    #ifdef __linux__
    // Si el usuario lanza la app con NVIDIA PRIME offload y no ha forzado un driver manual,
    // usamos XWayland (x11) para evitar el crash de dmabuf entre NVIDIA e Intel en Wayland.
    if (SDL_getenv("__NV_PRIME_RENDER_OFFLOAD") != nullptr && SDL_getenv("SDL_VIDEO_DRIVER") == nullptr) {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
    }
    #endif

    if (TryInitializeSDL(window, gl_context)) {
        return true;
    }

    #ifdef __linux__
    const char* envDriver = SDL_getenv("SDL_VIDEO_DRIVER");
    const char* hintDriver = SDL_GetHint(SDL_HINT_VIDEO_DRIVER);
    bool alreadyX11 = (envDriver != nullptr && std::string_view(envDriver) == "x11") ||
                      (hintDriver != nullptr && std::string_view(hintDriver) == "x11");
    if (!alreadyX11) {
        std::cerr << "Retrying graphics initialization with SDL_VIDEO_DRIVER=x11...\n";
        SDL_Quit();
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "x11", SDL_HINT_OVERRIDE);
        return TryInitializeSDL(window, gl_context);
    }
    #endif

    return false;
}

static void SetupStyle(){
    SetupImGuiStyle();
    SetupModernStyle();
    SetupFont();
}

static ImGuiStyle g_baseStyle;
static float g_baseFontScale = 1.0F;

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
    g_baseStyle = ImGui::GetStyle();
    g_baseFontScale = imgui_io.FontGlobalScale;
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

static void HandleSDLEvents(bool& done, SDL_Window* window, AppState& state) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT ||
            (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))) {
            if (Control::ShouldPromptCartridgeSDSave(state)) {
                state.emulator.Pause();
                state.popups.showCartridgeSavePrompt = true;
                state.popups.pendingCartridgeAction = Control::PendingCartridgeAction::Exit;
            } else {
                done = true;
            }
        }
    }
}

// --- Media Recording Logic moved to Frontend/UI/RecordingManager.cpp ---

static void Cleanup(AppState& state, SDL_Window* window, SDL_GLContext gl_context) {
    state.emulator.Stop();
    state.emulator.GetSID().Close();
    if (state.crtFilter.IsInitialized()) {
        state.crtFilter.Destroy();
    }
    if (state.render.vramTexture != 0U) {
        glDeleteTextures(1, &state.render.vramTexture);
        state.render.vramTexture = 0;
    }
    if (state.render.profilerTexture != 0U) {
        glDeleteTextures(1, &state.render.profilerTexture);
        state.render.profilerTexture = 0;
    }
    if (state.render.layoutTexture != 0U) {
        glDeleteTextures(1, &state.render.layoutTexture);
        state.render.layoutTexture = 0;
    }
    if (state.render.sidTexture != 0U) {
        glDeleteTextures(1, &state.render.sidTexture);
        state.render.sidTexture = 0;
    }
    if (state.render.sidFBO != 0U) {
        glDeleteFramebuffers(1, &state.render.sidFBO);
        state.render.sidFBO = 0;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
#ifndef TARGET_WASM
    CompilerVFS::Cleanup();
#endif
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

    HandleSDLEvents(done, window, state);

    if (state.requestExit) {
        done = true;
    }

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
    float top_section_height = work_size.y * 0.36F;
    if (work_size.y > 0.0F && work_size.x > 0.0F) {
        constexpr float target_aspect = 16.0F / 9.0F;
        const float ideal_height = work_size.x / target_aspect;
        const float missing_height = ideal_height - work_size.y;
        if (missing_height > 0.0F && missing_height <= ideal_height * 0.15F) {
            top_section_height = ideal_height * 0.36F;
        } else {
            const float current_aspect = work_size.x / work_size.y;
            if (current_aspect > target_aspect) {
                float target_width = work_size.y * target_aspect;
                work_pos.x += (work_size.x - target_width) * 0.5F;
                work_size.x = target_width;
            } else if (current_aspect < target_aspect) {
                float target_height = work_size.x / target_aspect;
                work_pos.y += (work_size.y - target_height) * 0.5F;
                work_size.y = target_height;
            }
            top_section_height = work_size.y * 0.36F;
        }

        const float ui_scale = std::max(0.25F, work_size.x / 1920.0F);
        state.render.uiScale = ui_scale;
        ImGuiIO& imgui_io = ImGui::GetIO();
        imgui_io.FontGlobalScale = g_baseFontScale * ui_scale;
        ImGui::GetStyle() = g_baseStyle;
        ImGui::GetStyle().ScaleAllSizes(ui_scale);
    }

    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

    Frontend::HandleDialogs(state);
    Frontend::DrawPopups(state);

    // Draw all windows
    Frontend::DrawGUIWindows(state, work_pos, work_size, top_section_height, windowFlags);

    ImGui::Render();
    ImGuiIO& imgui_io = ImGui::GetIO();
    glViewport(0, 0, (int)imgui_io.DisplaySize.x, (int)imgui_io.DisplaySize.y);
    glClearColor(0.0F, 0.0F, 0.0F, 1.00F);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifndef TARGET_WASM
    Frontend::UpdateMediaRecording(state, *args->mediaExporter);
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
    const ParseArgsResult parseResult = CliArgsParser::Parse(std::span<const char* const>(argv, static_cast<std::size_t>(argc)));
    if (parseResult.showUsage) {
        CliArgsParser::PrintUsage(argv[0]);
        return 0;
    }
    if (!parseResult.ok) {
        std::cerr << "Argument error: " << parseResult.error << "\n";
        CliArgsParser::PrintUsage(argv[0]);
        return -1;
    }
    const Args args = parseResult.args;

#ifndef TARGET_WASM
    Control::SymbolTable::SetElfLoader(&CompilerFrontend::LoadELF);
#endif

    if (args.headless) {
        int ret = HeadlessRunner::Run(args, sidSampleRate);
#ifndef TARGET_WASM
        CompilerVFS::Cleanup();
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
    SDKManager::CheckSDKStatus(state);
#endif

#ifndef TARGET_WASM
    // Check for updates
    UpdateChecker::CheckForUpdates(PROJECT_VERSION, [](bool available, const std::string& version) {
        if (available) {
            state.update.latestVersionTag = version;
            state.update.available = true;
        }
    });
#endif

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
    Console::SetInputResponseCallback([](char c) { state.emulator.InjectKey(c); });
    state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);

    static bool done = false;
    state.emulator.Start();

#if defined(__linux__) && !defined(TARGET_WASM)
    malloc_trim(0);
#endif
    
#ifndef TARGET_WASM
    static std::unique_ptr<MediaExporter> mediaExporter = nullptr;
#endif

#ifdef TARGET_WASM
    WebFileUtils::init_url_loader(&state);
    static MainLoopArgs loopArgs = { .window = window, .gl_context = gl_context, .state = &state, .mediaExporter = nullptr, .done = &done };
    emscripten_set_main_loop_arg(MainLoop, &loopArgs, 0, 1);
#else
    MainLoopArgs loopArgs = { .window = window, .gl_context = gl_context, .state = &state, .mediaExporter = &mediaExporter, .done = &done };
    int framesToRender = 2;

    const auto isEmulationActive = [](const AppState& s) {
        return (!s.emulator.IsPaused() && s.emulator.IsRunning()) ||
               s.emulation.isRecordingVideo ||
               (s.ide.open && s.ide.mode == Control::IDEMode::MIDI && GUI::IsMIDIAudioActive());
    };

    while (!done) {
        if (!isEmulationActive(state)) {
            if (framesToRender <= 0) {
#if defined(__linux__) && !defined(TARGET_WASM)
                malloc_trim(0);
#endif
                SDL_Event event;
                if (SDL_WaitEventTimeout(&event, 250)) {
                    ImGui_ImplSDL3_ProcessEvent(&event);
                    if (event.type == SDL_EVENT_QUIT ||
                        (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))) {
                        if (Control::ShouldPromptCartridgeSDSave(state)) {
                            state.emulator.Pause();
                            state.popups.showCartridgeSavePrompt = true;
                            state.popups.pendingCartridgeAction = Control::PendingCartridgeAction::Exit;
                        } else {
                            done = true;
                        }
                    }
                    framesToRender = 2;
                }
            }
            if (framesToRender > 0) {
                framesToRender--;
                MainLoop(&loopArgs);
            }
        } else {
            framesToRender = 2;
            MainLoop(&loopArgs);
        }
    }

    mediaExporter.reset();
    Cleanup(state, window, gl_context);
#endif

    return 0;
}
