#include <GL/glew.h>
#include <ImGuiFileDialog.h>
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <cstring>
#include <deque>
#include <iostream>
#include <string>

#include "../Componentes/Emulator.h"

Emulator emulator;
bool emulationRunning = false;
int instructionsPerFrame = 1000000;
bool autoOptimize = true;
std::deque<char> consoleOutput;
const size_t CONSOLE_MAX_SIZE = 20000;

void ClearConsole() {
    consoleOutput.clear();
}

void OutputCallback(char c) {
    consoleOutput.push_back(c);
    if (consoleOutput.size() > CONSOLE_MAX_SIZE) {
        // Remove entire lines from the top to avoid "cutting" characters
        char removed;
        do {
            if (consoleOutput.empty()) break;
            removed = consoleOutput.front();
            consoleOutput.pop_front();
        } while (removed != '\n');
    }
}

int main(int argc, char* argv[]) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
        0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

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
    SDL_Window* window =
        SDL_CreateWindow("65C02 Simulator", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);  // Enable vsync

    // Initialize OpenGL loader
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
        return 1;
    }

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
    std::string bin;
    bool romLoaded = false;
    if (argc > 1) {
        bin = argv[1];
        std::string errorMsg;
        if (emulator.Init(bin, errorMsg)) {
            romLoaded = true;
        } else {
            std::cerr << "Failed to load ROM from args: " << errorMsg
                      << std::endl;
        }
    }
    emulator.SetOutputCallback(OutputCallback);

    // Main loop
    bool done = false;
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

        // Auto-optimize IPS
        static Uint32 lastAdjustmentTime = 0;
        static float fpsSum = 0.0f;
        static int fpsCount = 0;

        if (autoOptimize && emulationRunning) {
            fpsSum += io.Framerate;
            fpsCount++;
            Uint32 currentTime = SDL_GetTicks();

            if (currentTime - lastAdjustmentTime > 250) {
                float avgFPS = fpsSum / fpsCount;
                fpsSum = 0;
                fpsCount = 0;
                lastAdjustmentTime = currentTime;

                // VSync target is ~60 FPS.
                if (avgFPS >= 59.5f) {
                    // Slowly increase IPS to probe limits.
                    instructionsPerFrame =
                        (int)(instructionsPerFrame * 1.01f) + 100;
                } else {
                    // Reduce IPS.
                    float drop = 60.0f - avgFPS;
                    if (drop < 0) drop = 0;

                    float factor = 1.0f - (drop / 120.0f);

                    if (factor < 0.5f)
                        factor = 0.5f;  // Clamp max reduction per step

                    instructionsPerFrame = (int)(instructionsPerFrame * factor);
                }

                // Clamp
                if (instructionsPerFrame < 100) instructionsPerFrame = 100;
                const int MAX_IPS = 1000000;
                if (instructionsPerFrame > MAX_IPS)
                    instructionsPerFrame = MAX_IPS;
            }
        } else {
            // Reset stats when not optimizing or paused to avoid stale data
            fpsSum = 0;
            fpsCount = 0;
            lastAdjustmentTime = SDL_GetTicks();
        }

        // Emulation Step
        if (emulationRunning) {
            for (int i = 0; i < instructionsPerFrame; ++i) {
                int res = emulator.Step();
                if (res != 0) {
                    emulationRunning = false;
                    if (res == 1) {
                        std::cerr << "CPU Stopped (STOP/JAM)" << std::endl;
                    } else {
                        std::cerr << "Emulator stopped (Code: " << res << ")"
                                  << std::endl;
                    }
                    break;
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Main Menu Bar
        float main_menu_height = 0.0f;
        if (ImGui::BeginMainMenuBar()) {
            main_menu_height = ImGui::GetWindowSize().y;
            if (ImGui::BeginMenu("Load ROM File")) {
                if (ImGui::MenuItem("Open ROM File...")) {
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "ChooseFileDlgKey", "Choose File", ".bin", ".");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Layout Configuration
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;
        work_pos.y += main_menu_height;
        work_size.y -= main_menu_height;

        float top_section_height = work_size.y * 0.25f;
        float bottom_section_height = work_size.y - top_section_height;
        const ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        // File Dialog
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName =
                    ImGuiFileDialog::Instance()->GetFilePathName();
                emulationRunning = false;
                std::string errorMsg;
                if (emulator.Init(filePathName, errorMsg)) {
                    bin = filePathName;
                    romLoaded = true;
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

        if (!romLoaded &&
            !ImGuiFileDialog::Instance()->IsOpened("ChooseFileDlgKey")) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
                                                    "Choose File", ".bin", ".");
        }

        // Control Window
        {
            ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(work_size.x * 0.75f, top_section_height * 0.4f),
                ImGuiCond_Always);
            ImGui::Begin("Control", nullptr,
                         window_flags | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::BeginDisabled(!romLoaded);
            if (ImGui::Button(emulationRunning ? "Pause" : "Run")) {
                emulationRunning = !emulationRunning;
            }
            ImGui::SameLine();
            if (ImGui::Button("Step")) {
                emulator.Step();
                emulationRunning = false;  // Stepping pauses automatic run
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                if (romLoaded) {
                    ClearConsole();
                    std::string errorMsg;
                    if (!emulator.Init(bin, errorMsg)) {
                        std::cerr << "Error resetting ROM: " << errorMsg
                                  << std::endl;
                        romLoaded = false;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(autoOptimize ? "Auto (On)" : "Auto (Off)")) {
                autoOptimize = !autoOptimize;
            }

            ImGui::SameLine();
            ImGui::Text("Avg %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate,
                        io.Framerate);

            ImGui::SliderInt("Speed (IPS)", &instructionsPerFrame, 1, 1000000);
            ImGui::SetScrollHereY(1.0f);
            ImGui::End();
        }

        // LCD Window
        {
            ImGui::SetNextWindowPos(
                ImVec2(work_pos.x, work_pos.y + top_section_height * 0.4f),
                ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(work_size.x * 0.75f, top_section_height * 0.6f),
                ImGuiCond_Always);
            ImGui::Begin("LCD Output", nullptr, window_flags);

            const auto& screen = emulator.GetLCDScreen();

            char line1[17];
            char line2[17];
            std::memcpy(line1, screen[0], 16);
            line1[16] = 0;
            std::memcpy(line2, screen[1], 16);
            line2[16] = 0;

            ImGui::Text("%s", line1);
            ImGui::Text("%s", line2);

            ImGui::End();
        }

        // Registers Window
        {
            ImGui::SetNextWindowPos(
                ImVec2(work_pos.x + work_size.x * 0.75f, work_pos.y),
                ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(work_size.x * 0.25f, top_section_height),
                ImGuiCond_Always);
            ImGui::Begin("Registers", nullptr, window_flags);
            const auto& cpu = emulator.GetCPU();
            ImGui::Text("PC: %04X", cpu.PC);
            ImGui::Text("SP: %04X", cpu.SP);
            ImGui::Text("A:  %02X", cpu.A);
            ImGui::Text("X:  %02X", cpu.X);
            ImGui::Text("Y:  %02X", cpu.Y);

            ImGui::Separator();
            ImGui::Text("Flags: %02X", cpu.GetStatus());
            ImGui::Text("N: %d V: %d B: %d D: %d I: %d Z: %d C: %d", cpu.N,
                        cpu.V, cpu.B, cpu.D, cpu.I, cpu.Z, cpu.C);
            ImGui::End();
        }

        // Console Window
        {
            ImGui::SetNextWindowPos(
                ImVec2(work_pos.x, work_pos.y + top_section_height),
                ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(work_size.x, bottom_section_height),
                                     ImGuiCond_Always);
            ImGui::Begin("Console", nullptr, window_flags);
            if (ImGui::IsWindowFocused()) {
                for (int n = 0; n < io.InputQueueCharacters.Size; n++) {
                    unsigned int c = io.InputQueueCharacters[n];
                    if (c > 0 && c < 0x80) {
                        emulator.InjectKey((char)c);
                    }
                }

                // Special keys
                if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
                    emulator.InjectKey(0x7F);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                    ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
                    emulator.InjectKey('\r');
                }
            }

            // Simple text dump
            for (char c : consoleOutput) {
                if (c == '\r') continue;
                if (c == '\n')
                    ImGui::TextUnformatted("\n");
                else {
                    char str[2] = {c, 0};
                    ImGui::TextUnformatted(str);
                    ImGui::SameLine();
                }
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);

            ImGui::End();
        }

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
