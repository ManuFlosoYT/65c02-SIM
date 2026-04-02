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
#include "Frontend/GUI/Style/Style.h"
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
#include "Frontend/UI/UIModules.h"

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

// --- Media Recording Logic moved to Frontend/UI/RecordingManager.cpp ---

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

