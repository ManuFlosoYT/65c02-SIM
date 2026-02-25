#include <ImGuiFileDialog.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <iostream>
#include <string>

#include "Frontend/Control/AppState.h"
#include "Frontend/Control/Console.h"
#include "Frontend/GUI/ConsoleWindow.h"
#include "Frontend/GUI/ControlWindow.h"
#include "Frontend/GUI/LCDWindow.h"
#include "Frontend/GUI/RegistersWindow.h"
#include "Frontend/GUI/SIDViewerWindow.h"
#include "Frontend/GUI/UpdatePopup.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "UpdateChecker.h"

using namespace Control;
using namespace Core;
using namespace Frontend;
using namespace Hardware;

int main(int argc, char* argv[]) {
    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    AppState state;
    state.emulator.GetSID().Init();

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                          SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_MAXIMIZED);
    SDL_Window* window = SDL_CreateWindow("65C02 Simulator " PROJECT_VERSION,
                                          1920, 1080, window_flags);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Check for updates
    UpdateChecker::CheckForUpdates(
        PROJECT_VERSION, [&state](bool available, const std::string& version) {
            if (available) {
                state.latestVersionTag = version;
                state.updateAvailable = true;
            }
        });

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        std::cerr << "Failed to create GL context: " << SDL_GetError()
                  << std::endl;
        return -1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);  // Enable vsync

    // Initialize OpenGL loader
    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL loader (gladLoadGL)!"
                  << std::endl;
        return 1;
    }

    // Create OpenGL texture for VRAM display
    glGenTextures(1, &state.vramTexture);
    glBindTexture(GL_TEXTURE_2D, state.vramTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    unsigned char emptyPixels[GPU::VRAM_HEIGHT * GPU::VRAM_WIDTH * 3] = {0};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GPU::VRAM_WIDTH, GPU::VRAM_HEIGHT, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, emptyPixels);

    glGenTextures(1, &state.profilerTexture);
    glBindTexture(GL_TEXTURE_2D, state.profilerTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &state.layoutTexture);
    glBindTexture(GL_TEXTURE_2D, state.layoutTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);

    state.crtFilter.Init(GPU::VRAM_WIDTH, GPU::VRAM_HEIGHT);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    float dpi_scale = SDL_GetWindowDisplayScale(window);
    if (dpi_scale <= 0.0f) dpi_scale = 1.0f;

    io.FontGlobalScale = 1.5f / dpi_scale;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(1.0f / dpi_scale);
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    if (argc > 1) {
        state.bin = argv[1];
        std::string errorMsg;
        if (state.emulator.Init(state.bin, errorMsg)) {
            state.romLoaded = true;
            state.emulator.ClearProfiler();
        } else {
            std::cerr << "Failed to load ROM from args: " << errorMsg
                      << std::endl;
        }
    }
    state.emulator.SetOutputCallback(Console::OutputCallback);
    state.emulator.SetGPUEnabled(state.gpuEnabled);

    bool done = false;
    state.emulator.Start();

    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    Uint64 frameStart;
    int frameTime;

    while (!done) {
        frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        if (!state.emulator.IsPaused()) {
            state.instructionsPerFrame = state.emulator.GetTargetIPS();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Layout Configuration
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;
        float top_section_height = work_size.y * 0.25f;

        const ImGuiWindowFlags wf = ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus;

        // File Dialog
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName =
                    ImGuiFileDialog::Instance()->GetFilePathName();
                state.emulator.Pause();
                std::string errorMsg;
                if (state.emulator.Init(filePathName, errorMsg)) {
                    state.bin = filePathName;
                    state.romLoaded = true;
                    state.emulator.SetGPUEnabled(state.gpuEnabled);
                    state.emulator.ClearProfiler();
                } else {
                    ImGui::OpenPopup("ErrorLoadingROM");
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

        // Save State Dialog
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGuiFileDialog::Instance()->Display("SaveStateDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName =
                    ImGuiFileDialog::Instance()->GetFilePathName();
                state.emulator.Pause();
                if (!state.emulator.SaveState(filePathName)) {
                    ImGui::OpenPopup("ErrorSavingState");
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

        // Load State Dialog
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGuiFileDialog::Instance()->Display("LoadStateDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName =
                    ImGuiFileDialog::Instance()->GetFilePathName();
                state.emulator.Pause();
                state.emulator.LoadState(filePathName,
                                         state.forceLoadSaveState);
                auto loadResult = state.emulator.GetLastLoadResult();
                bool loadedOk =
                    loadResult == SavestateLoadResult::Success ||
                    loadResult == SavestateLoadResult::VersionMismatch ||
                    loadResult == SavestateLoadResult::HashMismatch;

                if (loadedOk) {
                    state.romLoaded = true;
                    state.gpuEnabled = state.emulator.IsGPUEnabled();
                    state.instructionsPerFrame = state.emulator.GetTargetIPS();
                    state.cycleAccurate = state.emulator.IsCycleAccurate();
                    state.autoReload = state.emulator.IsAutoReloadEnabled();
                    state.bin = state.emulator.GetCurrentBinPath();
                }

                if (loadResult != SavestateLoadResult::Success) {
                    ImGui::OpenPopup("SavestateFeedback");
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGui::BeginPopupModal("ErrorLoadingROM", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Error loading ROM. Please check the file.");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("ErrorSavingState", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Error saving state. Please check your permissions.");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("SavestateFeedback", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            auto result = state.emulator.GetLastLoadResult();
            if (result == SavestateLoadResult::VersionMismatch ||
                result == SavestateLoadResult::HashMismatch) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                                   "Warning: Savestate compatibility issue");
                if (result == SavestateLoadResult::VersionMismatch) {
                    ImGui::Text("Version mismatch detected:");
                    ImGui::BulletText(
                        "Saved: %s",
                        state.emulator.GetLastLoadVersion().c_str());
                    ImGui::BulletText("Current: %s", PROJECT_VERSION);
                } else {
                    ImGui::Text(
                        "Hash mismatch. The data might be modified or "
                        "corrupt.");
                }
                ImGui::Text(
                    "The state was loaded, but some things might not work "
                    "correctly.");
            } else if (result == SavestateLoadResult::StructuralError) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                                   "Error: Failed to load state");
                ImGui::Text(
                    "The data is structurally incompatible or corrupted.");
            } else {
                ImGui::Text(
                    "An unknown error occurred while loading the state.");
            }

            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Draw all windows
        GUI::DrawUpdatePopup(state);
        GUI::DrawControlWindow(state, work_pos, work_size, top_section_height,
                               wf);
        GUI::DrawLCDWindow(state, work_pos, work_size, top_section_height, wf);
        GUI::DrawSIDViewerWindow(state, work_pos, work_size, top_section_height,
                                 wf);
        GUI::DrawRegistersWindow(state, work_pos, work_size, top_section_height,
                                 wf);
        GUI::DrawConsoleWindow(state, work_pos, work_size, top_section_height,
                               wf);
        state.crtTime = SDL_GetTicks() / 1000.0f;
        GUI::DrawVRAMViewerWindow(state, work_pos, work_size,
                                  top_section_height, wf);

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        frameTime = (int)(SDL_GetTicks() - frameStart);
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Cleanup
    state.emulator.Stop();
    state.emulator.GetSID().Close();
    state.crtFilter.Destroy();
    glDeleteTextures(1, &state.vramTexture);
    glDeleteTextures(1, &state.profilerTexture);
    glDeleteTextures(1, &state.layoutTexture);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
