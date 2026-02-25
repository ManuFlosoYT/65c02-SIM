#include "Frontend/GUI/Debugger/MemoryLayoutWindow.h"

#include <glad/gl.h>
#include <imgui.h>

#include <cmath>
#include <string>
#include <vector>

using namespace Control;
using namespace Hardware;

namespace GUI {

static ImVec4 GetDeviceColor(const std::string& name) {
    uint32_t hash = 0x811c9dc5;
    for (char c : name) {
        hash ^= (uint8_t)c;
        hash *= 0x01000193;
    }

    double h = std::fmod((double)hash * 0.618033988749895, 1.0);
    double s = 0.6 + std::fmod((double)hash * 0.1, 0.3);
    double v = 0.8 + std::fmod((double)hash * 0.05, 0.2);

    float r, g, b;
    ImGui::ColorConvertHSVtoRGB((float)h, (float)s, (float)v, r, g, b);
    return ImVec4(r, g, b, 0.6f);
}

void DrawMemoryLayoutWindow(AppState& state) {
    auto& bus = state.emulator.GetMem();
    const auto& devices = bus.GetRegisteredDevices();

    static unsigned char pixels[256 * 256 * 3];
    for (int i = 0; i < 256 * 256 * 3; i++) pixels[i] = 0;

    for (const auto& reg : devices) {
        if (!reg.enabled) continue;

        ImVec4 color = GetDeviceColor(reg.device->GetName());
        unsigned char r = (unsigned char)(color.x * 255);
        unsigned char g = (unsigned char)(color.y * 255);
        unsigned char b = (unsigned char)(color.z * 255);

        for (int addr = reg.startAddress; addr <= (int)reg.endAddress; ++addr) {
            int idx = addr * 3;
            if (pixels[idx] != 0 || pixels[idx + 1] != 0 ||
                pixels[idx + 2] != 0) {
                pixels[idx] = (pixels[idx] + r) / 2;
                pixels[idx + 1] = (pixels[idx + 1] + g) / 2;
                pixels[idx + 2] = (pixels[idx + 2] + b) / 2;
            } else {
                pixels[idx] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, state.layoutTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE,
                    pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    ImGui::Columns(2, "LayoutColumns", true);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.4f);

    ImGui::Text("Memory Map Visualization");
    ImGui::Separator();
    float mapSize = ImGui::GetContentRegionAvail().x;
    if (mapSize > ImGui::GetContentRegionAvail().y - 40)
        mapSize = ImGui::GetContentRegionAvail().y - 40;

    ImGui::Image((ImTextureID)(intptr_t)state.layoutTexture,
                 ImVec2(mapSize, mapSize));

    if (ImGui::IsItemHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 imgPos = ImGui::GetItemRectMin();
        ImVec2 imgSize = ImGui::GetItemRectSize();
        int memX = (int)((mousePos.x - imgPos.x) / imgSize.x * 256.0f);
        int memY = (int)((mousePos.y - imgPos.y) / imgSize.y * 256.0f);
        Word addr = (memY << 8) | memX;

        if (ImGui::BeginItemTooltip()) {
            ImGui::Text("Address: 0x%04X", addr);
            for (const auto& reg : devices) {
                if (reg.enabled && addr >= reg.startAddress &&
                    addr <= reg.endAddress) {
                    ImGui::TextColored(GetDeviceColor(reg.device->GetName()),
                                       "- %s", reg.device->GetName().c_str());
                }
            }
            ImGui::EndTooltip();
        }
    }

    ImGui::NextColumn();

    ImGui::Text("Hardware Components");
    ImGui::Separator();

    static char collisionError[256] = "";
    static bool showCollisionError = false;

    if (ImGui::BeginTable("DeviceTable", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed,
                                80);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed,
                                25);
        ImGui::TableSetupColumn("Ignored", ImGuiTableColumnFlags_WidthFixed,
                                25);
        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 25);
        ImGui::TableHeadersRow();

        int devIdx = 0;
        for (auto& reg :
             const_cast<std::vector<DeviceRegistration>&>(devices)) {
            ImGui::PushID(devIdx);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", reg.device->GetName().c_str());

            ImGui::TableSetColumnIndex(1);
            int base = reg.startAddress;
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##base", &base, 1.0f, 0, 0xFFFF, "0x%04X")) {
                int size = (reg.endAddress - reg.startAddress) + 1;
                reg.startAddress = (Word)base;
                reg.endAddress = (Word)(base + size - 1);
            }
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(2);
            int size = (reg.endAddress - reg.startAddress) + 1;
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##size", &size, 1.0f, 1, 0x10000, "0x%04X")) {
                reg.endAddress = (Word)(reg.startAddress + size - 1);
            }
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(3);
            ImGui::Checkbox("##en", &reg.enabled);

            ImGui::TableSetColumnIndex(4);
            ImGui::Checkbox("##ign", &reg.ignoreCollision);

            ImGui::TableSetColumnIndex(5);
            ImGui::ColorButton("##device_color",
                               GetDeviceColor(reg.device->GetName()),
                               ImGuiColorEditFlags_NoTooltip);

            ImGui::PopID();
            devIdx++;
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("Apply & Reset Emulator", ImVec2(-1, 40))) {
        try {
            bus.RebuildDeviceMap();

            std::string error;
            if (!state.emulator.Init(state.bin, error)) {
                strncpy(collisionError, error.c_str(), 255);
                showCollisionError = true;
            }
        } catch (const std::runtime_error& e) {
            strncpy(collisionError, e.what(), 255);
            showCollisionError = true;
        }
    }

    if (showCollisionError) {
        ImGui::OpenPopup("Collision Error");
        showCollisionError = false;
    }

    if (ImGui::BeginPopupModal("Collision Error", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Hardware Configuration Error:");
        ImGui::TextWrapped("%s", collisionError);
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Columns(1);
}

}  // namespace GUI
