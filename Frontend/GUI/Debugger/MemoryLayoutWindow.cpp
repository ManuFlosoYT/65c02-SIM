#include "Frontend/GUI/Debugger/MemoryLayoutWindow.h"

#include <glad/gl.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace Control;
using namespace Hardware;

namespace GUI {

static ImVec4 GetDeviceColor(const std::string& name) {
    uint32_t hash = 0x811c9dc5;
    for (char character : name) {
        hash ^= static_cast<uint8_t>(character);
        hash *= 0x01000193;
    }

    double hue = std::fmod(static_cast<double>(hash) * 0.618033988749895, 1.0);
    double sat = 0.6 + std::fmod(static_cast<double>(hash) * 0.1, 0.3);
    double val = 0.8 + std::fmod(static_cast<double>(hash) * 0.05, 0.2);

    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
    ImGui::ColorConvertHSVtoRGB(static_cast<float>(hue), static_cast<float>(sat), static_cast<float>(val), red, green,
                                blue);
    return {red, green, blue, 0.6F};
}

namespace {

void UpdateMemoryLayoutTexture(AppState& state, const std::vector<DeviceRegistration>& devices,
                               std::array<unsigned char, 256ULL * 256ULL * 3ULL>& pixels) {
    pixels.fill(0);

    for (const auto& reg : devices) {
        if (!reg.enabled) {
            continue;
        }

        ImVec4 color = GetDeviceColor(reg.device->GetName());
        auto red = static_cast<unsigned char>(color.x * 255.0F);
        auto green = static_cast<unsigned char>(color.y * 255.0F);
        auto blue = static_cast<unsigned char>(color.z * 255.0F);

        for (int addr = reg.startAddress; addr <= static_cast<int>(reg.endAddress); ++addr) {
            std::size_t idx = static_cast<std::size_t>(addr) * 3U;
            if (pixels.at(idx) != 0 || pixels.at(idx + 1) != 0 || pixels.at(idx + 2) != 0) {
                pixels.at(idx) = static_cast<unsigned char>((pixels.at(idx) + red) / 2);
                pixels.at(idx + 1) = static_cast<unsigned char>((pixels.at(idx + 1) + green) / 2);
                pixels.at(idx + 2) = static_cast<unsigned char>((pixels.at(idx + 2) + blue) / 2);
            } else {
                pixels.at(idx) = red;
                pixels.at(idx + 1) = green;
                pixels.at(idx + 2) = blue;
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, state.layoutTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawMemoryLayoutTooltip(const std::vector<DeviceRegistration>& devices, float imgSizeX, float imgSizeY) {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 imgPos = ImGui::GetItemRectMin();

    int memX = static_cast<int>((mousePos.x - imgPos.x) / imgSizeX * 256.0F);
    int memY = static_cast<int>((mousePos.y - imgPos.y) / imgSizeY * 256.0F);
    memX = std::clamp(memX, 0, 255);
    memY = std::clamp(memY, 0, 255);

    Word addr = static_cast<Word>((memY << 8) | memX);

    if (ImGui::BeginItemTooltip()) {
        std::ostringstream addrBuf;
        addrBuf << "Address: 0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << addr;
        ImGui::TextUnformatted(addrBuf.str().c_str());

        for (const auto& reg : devices) {
            if (reg.enabled && addr >= reg.startAddress && addr <= reg.endAddress) {
                ImGui::PushStyleColor(ImGuiCol_Text, GetDeviceColor(reg.device->GetName()));

                std::ostringstream txtBuf;
                txtBuf << "- " << reg.device->GetName();
                ImGui::TextUnformatted(txtBuf.str().c_str());

                ImGui::PopStyleColor();
            }
        }
        ImGui::EndTooltip();
    }
}

void DrawHardwareComponentsTable(std::vector<DeviceRegistration>& devices) {
    ImGui::TextUnformatted("Hardware Components");
    ImGui::Separator();

    if (ImGui::BeginTable("DeviceTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, 25);
        ImGui::TableSetupColumn("Ignored", ImGuiTableColumnFlags_WidthFixed, 25);
        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 25);
        ImGui::TableHeadersRow();

        int devIdx = 0;
        for (auto& reg : devices) {
            ImGui::PushID(devIdx);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(reg.device->GetName().c_str());

            ImGui::TableSetColumnIndex(1);
            int base = reg.startAddress;
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##base", &base, 1.0F, 0, 0xFFFF, "0x%04X")) {
                int size = (reg.endAddress - reg.startAddress) + 1;
                reg.startAddress = static_cast<Word>(base);
                reg.endAddress = static_cast<Word>(base + size - 1);
            }
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(2);
            int size = (reg.endAddress - reg.startAddress) + 1;
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##size", &size, 1.0F, 1, 0x10000, "0x%04X")) {
                reg.endAddress = static_cast<Word>(reg.startAddress + size - 1);
            }
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(3);
            ImGui::Checkbox("##en", &reg.enabled);

            ImGui::TableSetColumnIndex(4);
            ImGui::Checkbox("##ign", &reg.ignoreCollision);

            ImGui::TableSetColumnIndex(5);
            ImGui::ColorButton("##device_color", GetDeviceColor(reg.device->GetName()), ImGuiColorEditFlags_NoTooltip);

            ImGui::PopID();
            devIdx++;
        }
        ImGui::EndTable();
    }
}

}  // namespace

void DrawMemoryLayoutWindow(AppState& state) {
    auto& bus = state.emulator.GetMem();
    const auto& devicesConst = bus.GetRegisteredDevices();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto& devices = const_cast<std::vector<DeviceRegistration>&>(devicesConst);

    static std::array<unsigned char, 256ULL * 256ULL * 3ULL> pixels{};
    UpdateMemoryLayoutTexture(state, devices, pixels);

    ImGui::Columns(2, "LayoutColumns", true);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.4F);

    ImGui::TextUnformatted("Memory Map Visualization");
    ImGui::Separator();
    float mapSize = ImGui::GetContentRegionAvail().x;
    mapSize = std::min(mapSize, ImGui::GetContentRegionAvail().y - 40.0F);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<unsigned long long>(state.layoutTexture)),
                 ImVec2(mapSize, mapSize));

    if (ImGui::IsItemHovered()) {
        DrawMemoryLayoutTooltip(devices, mapSize, mapSize);
    }

    ImGui::NextColumn();

    DrawHardwareComponentsTable(devices);

    ImGui::Spacing();
    ImGui::Separator();

    static std::array<char, 256> collisionError{};
    static bool showCollisionError = false;

    if (ImGui::Button("Apply & Reset Emulator", ImVec2(-1, 40))) {
        try {
            bus.RebuildDeviceMap();

            std::string error;
            if (!state.emulator.Init(state.bin, error)) {
                strncpy(collisionError.data(), error.c_str(), 255);
                showCollisionError = true;
            }
        } catch (const std::runtime_error& e) {
            strncpy(collisionError.data(), e.what(), 255);
            showCollisionError = true;
        }
    }

    if (showCollisionError) {
        ImGui::OpenPopup("Collision Error");
        showCollisionError = false;
    }

    if (ImGui::BeginPopupModal("Collision Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("Hardware Configuration Error:");
        ImGui::PopStyleColor();
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextUnformatted(collisionError.data());
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Columns(1);
}

}  // namespace GUI
