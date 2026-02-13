#include <GL/glew.h>
#include <ImGuiFileDialog.h>
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Hardware/Emulator.h"

Emulator emulator;
int instructionsPerFrame = 1000000;
float ipsLogScale = 6.0f;
bool gpuEnabled = false;

std::vector<std::string> consoleLines;
std::string currentLine;
int cursorX = 0;
const size_t CONSOLE_MAX_LINES = 1000;

void ClearConsole() {
    consoleLines.clear();
    currentLine.clear();
    cursorX = 0;
}

void OutputCallback(char c) {
    if (c == '\r') {
        cursorX = 0;
    } else if (c == '\n') {
        consoleLines.push_back(currentLine);
        if (consoleLines.size() > CONSOLE_MAX_LINES) {
            consoleLines.erase(consoleLines.begin());
        }
        currentLine.clear();
        cursorX = 0;
    } else if (c == 0x08 || c == 0x7F) {  // Backspace
        if (cursorX > 0) cursorX--;
    } else if (c >= 32) {
        if (cursorX >= (int)currentLine.size()) {
            currentLine.resize(cursorX + 1, ' ');
        }
        currentLine[cursorX] = c;
        cursorX++;
    }
}

int main(int argc, char* argv[]) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER |
                 SDL_INIT_AUDIO) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    emulator.GetSID().Init();

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

    // Create OpenGL texture for VRAM display
    GLuint vramTexture;
    glGenTextures(1, &vramTexture);
    glBindTexture(GL_TEXTURE_2D, vramTexture);
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
    emulator.SetGPUEnabled(gpuEnabled);

    // Main loop
    bool done = false;
    emulator.Start();  // Start the thread (it will start paused)
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

        if (!emulator.IsPaused()) {
            instructionsPerFrame = emulator.GetTargetIPS();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Layout Configuration
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;

        float left_width = work_size.x * 0.5f;
        float right_width = work_size.x - left_width;
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
                emulator.Pause();
                std::string errorMsg;
                if (emulator.Init(filePathName, errorMsg)) {
                    bin = filePathName;
                    romLoaded = true;
                    emulator.SetGPUEnabled(gpuEnabled);
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

        // Control Window
        {
            float mainColWidth = emulator.GetSID().IsSoundEnabled()
                                     ? work_size.x * 0.375f
                                     : work_size.x * 0.75f;

            ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(mainColWidth, top_section_height * 0.4f),
                ImGuiCond_Always);
            ImGui::Begin("Control", nullptr,
                         window_flags | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::BeginDisabled(!romLoaded);
            if (ImGui::Button(emulator.IsPaused() ? "Run" : "Pause")) {
                if (emulator.IsPaused())
                    emulator.Resume();
                else
                    emulator.Pause();
                emulator.GetSID().SetEmulationPaused(emulator.IsPaused());
            }
            ImGui::SameLine();
            if (ImGui::Button("Step")) {
                emulator.GetSID().SetEmulationPaused(false);
                emulator.Pause();  // Ensure it is paused
                emulator.Step();
                emulator.GetSID().SetEmulationPaused(true);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                if (romLoaded) {
                    // 1. Pause execution to stop the thread loop safely
                    bool wasRunning = !emulator.IsPaused();
                    if (wasRunning) emulator.Pause();

                    // 2. Reset the emulator state
                    ClearConsole();
                    std::string errorMsg;
                    if (!emulator.Init(bin, errorMsg)) {
                        std::cerr << "Error resetting ROM: " << errorMsg
                                  << std::endl;
                        romLoaded = false;
                        ImGuiFileDialog::Instance()->OpenDialog(
                            "ChooseFileDlgKey", "Choose File", ".bin", ".");
                    } else {
                        emulator.SetGPUEnabled(gpuEnabled);
                        emulator.GetGPU().Init();
                    }

                    // 3. Resume if it was running before
                    if (wasRunning) {
                        emulator.Resume();
                        emulator.GetSID().SetEmulationPaused(false);
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(gpuEnabled ? "GPU (On)" : "GPU (Off)")) {
                gpuEnabled = !gpuEnabled;
                emulator.SetGPUEnabled(gpuEnabled);
            }
            ImGui::SameLine();
            bool soundEnabled = emulator.GetSID().IsSoundEnabled();
            if (ImGui::Button(soundEnabled ? "Sound (On)" : "Sound (Off)")) {
                emulator.GetSID().EnableSound(!soundEnabled);
            }
            ImGui::SameLine();
            ImGui::Text("Actual: %d IPS", emulator.GetActualIPS());

            int tempIPS = instructionsPerFrame;
            if (ImGui::InputInt("Target IPS", &tempIPS, 100000, 100000)) {
                if (instructionsPerFrame == 1 && tempIPS == 100001)
                    tempIPS = 100000;

                if (tempIPS < 1) tempIPS = 1;
                if (tempIPS > 1000000) tempIPS = 1000000;
                instructionsPerFrame = tempIPS;
                emulator.SetTargetIPS(instructionsPerFrame);
            }
            ImGui::SetScrollHereY(1.0f);
            ImGui::End();
        }

        // LCD Window
        {
            ImGui::SetNextWindowPos(
                ImVec2(work_pos.x, work_pos.y + top_section_height * 0.4f),
                ImGuiCond_Always);
            float lcdWidth = emulator.GetSID().IsSoundEnabled()
                                 ? work_size.x * 0.375f
                                 : work_size.x * 0.75f;
            ImGui::SetNextWindowSize(
                ImVec2(lcdWidth, top_section_height * 0.6f), ImGuiCond_Always);
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

        // SID Viewer Window
        if (emulator.GetSID().IsSoundEnabled()) {
            ImGui::SetNextWindowPos(
                ImVec2(work_pos.x + work_size.x * 0.375f, work_pos.y),
                ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(work_size.x * 0.375f, top_section_height),
                ImGuiCond_Always);
            ImGui::Begin("SID Viewer", nullptr, window_flags);

            for (int i = 0; i < 3; ++i) {
                const auto& voice = emulator.GetSID().GetVoice(i);
                ImGui::PushID(i);
                if (i > 0) ImGui::Separator();

                // Left Column: Name + Graph
                ImGui::BeginGroup();
                ImGui::Text("Voice %d", i + 1);

                // Waveform visualization
                float points[50];
                double time = ImGui::GetTime();
                float speed = 2.0f;  // Animation speed
                float freqScale = 1.0f + (voice.frequency / 4000.0f);

                for (int n = 0; n < 50; n++) {
                    float t = (float)n / 49.0f;
                    float phase =
                        (t * freqScale) - (float)(time * speed * freqScale);
                    phase -= floor(phase);

                    float val = 0.0f;
                    if (voice.control & 0x10) {  // Triangle
                        val = (phase < 0.5f) ? (-1.0f + 4.0f * phase)
                                             : (3.0f - 4.0f * phase);
                    } else if (voice.control & 0x20) {  // Sawtooth
                        val = 2.0f * phase - 1.0f;
                    } else if (voice.control & 0x40) {  // Pulse
                        float pw = (voice.pulseWidth & 0xFFF) / 4095.0f;
                        if (pw == 0) pw = 0.5f;
                        val = (phase < pw) ? 1.0f : -1.0f;
                    } else if (voice.control & 0x80) {  // Noise
                        val = ((float)(rand() % 100) / 50.0f) - 1.0f;
                    }
                    points[n] = val;
                }
                ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)i),
                                  ImVec2(100, 30), false,
                                  ImGuiWindowFlags_NoInputs);
                ImGui::PlotLines("##Wave", points, 50, 0, nullptr, -1.0f, 1.0f,
                                 ImVec2(100, 30));
                ImGui::EndChild();
                ImGui::EndGroup();

                ImGui::SameLine();

                // Right Column: Parameters
                ImGui::BeginGroup();
                ImGui::Text("Freq: %04X  PW: %03X", voice.frequency,
                            voice.pulseWidth);
                ImGui::Text("A:%X D:%X S:%X R:%X", voice.env.attackRate,
                            voice.env.decayRate,
                            (int)(voice.env.sustainLevel * 15),
                            voice.env.releaseRate);
                ImGui::SameLine();
                ImGui::Text("Ctrl: %02X (G:%d S:%d R:%d T:%d)", voice.control,
                            (voice.control & 1), (voice.control & 2) >> 1,
                            (voice.control & 4) >> 2, (voice.control & 8) >> 3);
                ImGui::EndGroup();
                ImGui::PopID();
            }
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
            ImGui::SetNextWindowSize(
                ImVec2(gpuEnabled ? left_width : work_size.x,
                       bottom_section_height),
                ImGuiCond_Always);
            ImGui::Begin("Console", nullptr, window_flags);

            if (ImGui::Button("Load ROM")) {
                if (!ImGuiFileDialog::Instance()->IsOpened()) {
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "ChooseFileDlgKey", "Choose File", ".bin", ".");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy Output")) {
                ImGui::OpenPopup("CopyConsoleOutput");
            }
            ImGui::Separator();

            if (ImGui::IsWindowFocused()) {
                for (int n = 0; n < io.InputQueueCharacters.Size; n++) {
                    unsigned int c = io.InputQueueCharacters[n];
                    if (c > 0 && c < 0x80) {
                        if (c == '\r' || c == '\n')
                            continue;  // Handled separately
                        emulator.InjectKey((char)c);
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
                    emulator.InjectKey(0x7F);
                if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                    ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
                    emulator.InjectKey('\r');
            }

            for (const auto& line : consoleLines) {
                ImGui::TextUnformatted(line.c_str());
            }
            ImGui::TextUnformatted(currentLine.c_str());

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20)
                ImGui::SetScrollHereY(1.0f);

            // Copy Output Modal
            if (ImGui::BeginPopupModal("CopyConsoleOutput", NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize)) {
                static std::string fullText;
                if (fullText.empty() || fullText.length() < 10) {
                    fullText.clear();
                    for (const auto& line : consoleLines)
                        fullText += line + "\n";
                    fullText += currentLine;
                }

                std::vector<char> buffer(fullText.begin(), fullText.end());
                buffer.push_back(0);

                ImGui::InputTextMultiline("##CopySource", buffer.data(),
                                          buffer.size(), ImVec2(800, 600),
                                          ImGuiInputTextFlags_ReadOnly);

                if (ImGui::Button("Close", ImVec2(120, 0))) {
                    fullText.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::End();
        }

        // VRAM Viewer Window
        if (gpuEnabled) {
            ImGui::SetNextWindowPos(ImVec2(work_pos.x + left_width,
                                           work_pos.y + top_section_height),
                                    ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(right_width, bottom_section_height),
                                     ImGuiCond_Always);
            ImGui::Begin("VRAM Viewer", nullptr, window_flags);

            if (ImGui::Button("Load Image")) {
                if (!ImGuiFileDialog::Instance()->IsOpened()) {
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "ChooseVRAMImageKey", "Choose VRAM Image", ".bin", ".",
                        "");
                }
            }

            ImGui::Separator();

            GPU& gpu = emulator.GetGPU();
            unsigned char pixels[GPU::VRAM_HEIGHT * GPU::VRAM_WIDTH * 3];
            for (int y = 0; y < GPU::VRAM_HEIGHT; y++) {
                for (int x = 0; x < GPU::VRAM_WIDTH; x++) {
                    int idx = (y * GPU::VRAM_WIDTH + x) * 3;
                    Byte val = gpu.vram[y][x];
                    pixels[idx + 0] = ((val >> 4) & 0x03) * 85;
                    pixels[idx + 1] = ((val >> 2) & 0x03) * 85;
                    pixels[idx + 2] = (val & 0x03) * 85;
                }
            }
            glBindTexture(GL_TEXTURE_2D, vramTexture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GPU::VRAM_WIDTH,
                            GPU::VRAM_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels);
            glBindTexture(GL_TEXTURE_2D, 0);

            ImVec2 avail = ImGui::GetContentRegionAvail();
            int scaleX = (int)(avail.x / GPU::VRAM_WIDTH);
            int scaleY = (int)(avail.y / GPU::VRAM_HEIGHT);
            int scale = scaleX < scaleY ? scaleX : scaleY;
            if (scale < 1) scale = 1;

            float imgW = (float)(GPU::VRAM_WIDTH * scale);
            float imgH = (float)(GPU::VRAM_HEIGHT * scale);
            float offsetX = (avail.x - imgW) * 0.5f;
            float offsetY = (avail.y - imgH) * 0.5f;
            if (offsetX > 0)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
            if (offsetY > 0)
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

            ImGui::Image((ImTextureID)(intptr_t)vramTexture,
                         ImVec2(imgW, imgH));
            ImGui::End();
        }

        // VRAM Image Load Dialog
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGuiFileDialog::Instance()->Display("ChooseVRAMImageKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string imgPath =
                    ImGuiFileDialog::Instance()->GetFilePathName();
                FILE* f = fopen(imgPath.c_str(), "rb");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long fileSize = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    unsigned char* buf = new unsigned char[fileSize];
                    fread(buf, 1, fileSize, f);
                    fclose(f);
                    GPU& gpu = emulator.GetGPU();
                    for (int y = 0; y < GPU::VRAM_HEIGHT; y++) {
                        for (int x = 0; x < GPU::VRAM_WIDTH; x++) {
                            size_t addr = (size_t)y * 128 + x;
                            if (addr < (size_t)fileSize)
                                gpu.vram[y][x] = buf[addr];
                        }
                    }
                    delete[] buf;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    emulator.Stop();
    glDeleteTextures(1, &vramTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
