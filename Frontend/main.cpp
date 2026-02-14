#include <GL/glew.h>
#include <ImGuiFileDialog.h>
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

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
#include "Frontend/GUI/VRAMViewerWindow.h"
#include "UpdateChecker.h"

using namespace Control;
using namespace Core;
using namespace Frontend;
using namespace Hardware;

int main(int argc, char* argv[]) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER |
                 SDL_INIT_AUDIO) != 0) {
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
                          SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(
        "65C02 Simulator " PROJECT_VERSION, SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);

    // Check for updates
    UpdateChecker::CheckForUpdates(
        PROJECT_VERSION, [&state](bool available, const std::string& version) {
            if (available) {
                state.latestVersionTag = version;
                state.updateAvailable = true;
            }
        });

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);  // Enable vsync

    // Initialize OpenGL loader
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
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
    glBindTexture(GL_TEXTURE_2D, 0);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 1.5f;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load emulator
    if (argc > 1) {
        state.bin = argv[1];
        std::string errorMsg;
        if (state.emulator.Init(state.bin, errorMsg)) {
            state.romLoaded = true;
        } else {
            std::cerr << "Failed to load ROM from args: " << errorMsg
                      << std::endl;
        }
    }
    state.emulator.SetOutputCallback(Console::OutputCallback);
    state.emulator.SetGPUEnabled(state.gpuEnabled);

    // Main loop
    bool done = false;
    state.emulator.Start();
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        if (!state.emulator.IsPaused()) {
            state.instructionsPerFrame = state.emulator.GetTargetIPS();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
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
                } else {
                    ImGui::OpenPopup("ErrorLoadingROM");
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
        GUI::DrawVRAMViewerWindow(state, work_pos, work_size,
                                  top_section_height, wf);

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    state.emulator.Stop();
    glDeleteTextures(1, &state.vramTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
