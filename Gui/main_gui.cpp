#include <GL/glew.h>
#include <ImGuiFileDialog.h>
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <deque>
#include <iostream>
#include <string>

#include "../Componentes/Emulator.h"

Emulator emulator;
bool emulationRunning = false;
int instructionsPerFrame = 1000;
std::deque<char> consoleOutput;
const size_t CONSOLE_MAX_SIZE = 2000;

void OutputCallback(char c) {
    consoleOutput.push_back(c);
    if (consoleOutput.size() > CONSOLE_MAX_SIZE) {
        consoleOutput.pop_front();
    }
}

int main(int argc, char* argv[]) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("65C02 Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
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

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load emulator
    std::string bin = "eater.bin"; // TODO: Remove this hardcoding
    if (argc > 1) {
        bin = argv[1];
    }
    emulator.Init(bin);
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

        // Emulation Step
        if (emulationRunning) {
            for (int i = 0; i < instructionsPerFrame; ++i) {
                emulator.Step();
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Main Menu Bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open File...")) {
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "ChooseFileDlgKey", "Choose File", ".bin", ".");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // File Dialog
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName =
                    ImGuiFileDialog::Instance()->GetFilePathName();
                // Load the new binary
                // Stop emulation first if running
                emulationRunning = false;
                emulator.Init(filePathName);
            }
            ImGuiFileDialog::Instance()->Close();
        }

        // Control Window
        {
            ImGui::Begin("Control");
            if (ImGui::Button(emulationRunning ? "Pause" : "Run")) {
                emulationRunning = !emulationRunning;
            }
            ImGui::SameLine();
            if (ImGui::Button("Step")) {
                emulator.Step();
                emulationRunning = false;  // Stepping pauses automatic run
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                emulator.Init(bin);
            }

            ImGui::SliderInt("Speed (Instructions per frame)", &instructionsPerFrame, 1, 1000000);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Registers Window
        {
            ImGui::Begin("Registers");
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
            ImGui::Begin("Console");
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
