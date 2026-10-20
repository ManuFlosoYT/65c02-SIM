#include "Frontend/GUI/Debugger/MemoryLayoutWindow.h"

#include <glad/gl.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace Control;
using namespace Hardware;
using namespace Core;

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
        if (!reg.enabled || reg.isVirtual) {
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

    glBindTexture(GL_TEXTURE_2D, state.render.layoutTexture);
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

void DrawMappedDevicesTable(AppState& state, std::vector<DeviceRegistration>& devices, bool& layoutDirty) {
    ImGui::TextUnformatted("Hardware Components");
    ImGui::Separator();

    bool isCartridgeLoaded = state.emulator.GetCartridge().loaded;
    if (isCartridgeLoaded) {
        ImGui::BeginDisabled(true);
    }

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
            if (reg.isVirtual) {
                devIdx++;
                continue;
            }
            ImGui::PushID(devIdx);
            ImGui::TableNextRow();

            bool isESP = (reg.device->GetName() == "ESP8266");
#ifdef TARGET_WASM
            if (isESP) {
                ImGui::BeginDisabled(true);
            }
#endif

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(reg.device->GetName().c_str());
#ifdef TARGET_WASM
            if (isESP && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("The ESP8266 WiFi chip is only supported in native/local builds.");
            }
#endif

            ImGui::TableSetColumnIndex(1);
            int base = reg.startAddress;
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##base", &base, 1.0F, 0, 0xFFFF, "0x%04X")) {
                int size = (reg.endAddress - reg.startAddress) + 1;
                reg.startAddress = static_cast<Word>(base);
                reg.endAddress = static_cast<Word>(base + size - 1);
                layoutDirty = true;
            }
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(2);
            int size = (reg.endAddress - reg.startAddress) + 1;
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##size", &size, 1.0F, 1, 0x10000, "0x%04X")) {
                reg.endAddress = static_cast<Word>(reg.startAddress + size - 1);
                layoutDirty = true;
            }
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(3);
            if (ImGui::Checkbox("##en", &reg.enabled)) {
                if (reg.device->GetName() == "SD Card") {
                    state.emulator.SetSDEnabled(reg.enabled);
                    state.emulation.sdEnabled = reg.enabled;
                } else if (reg.device->GetName() == "ESP8266") {
                    state.emulator.SetESPEnabled(reg.enabled);
                    state.emulation.espEnabled = reg.enabled;
                }
                layoutDirty = true;
            }

            ImGui::TableSetColumnIndex(4);
            if (ImGui::Checkbox("##ign", &reg.ignoreCollision)) {
                layoutDirty = true;
            }

            ImGui::TableSetColumnIndex(5);
            ImGui::ColorButton("##device_color", GetDeviceColor(reg.device->GetName()), ImGuiColorEditFlags_NoTooltip);

#ifdef TARGET_WASM
            if (isESP) {
                ImGui::EndDisabled();
            }
#endif

            ImGui::PopID();
            devIdx++;
        }
        ImGui::EndTable();
    }

    if (isCartridgeLoaded) {
        ImGui::EndDisabled();
    }
}

struct InspectorState {
    uint16_t startAddr = 0x0000;
    uint16_t endAddr = 0x00FF;
    int sizeMode = 0;
    int signedIntMode = 0;
    int selectedDeviceIdx = -1;
};

void RenderText(std::string_view text) { ImGui::TextUnformatted(text.data(), text.data() + text.size()); }

void RenderTextDisabled(std::string_view text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    ImGui::PopStyleColor();
}

void DrawInspectorRangeControls(const std::vector<DeviceRegistration>& devices, InspectorState& state) {
    ImGui::TextUnformatted("Range Selection");

    RenderTextDisabled("Presets:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Zero Page")) {
        state.startAddr = 0x0000;
        state.endAddr = 0x00FF;
        state.selectedDeviceIdx = -1;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Stack")) {
        state.startAddr = 0x0100;
        state.endAddr = 0x01FF;
        state.selectedDeviceIdx = -1;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Vectors")) {
        state.startAddr = 0xFFFA;
        state.endAddr = 0xFFFF;
        state.selectedDeviceIdx = -1;
    }

    std::vector<const DeviceRegistration*> validDevices;
    validDevices.reserve(devices.size());
    for (const auto& reg : devices) {
        if (reg.enabled && !reg.isVirtual) {
            validDevices.push_back(&reg);
        }
    }

    if (!validDevices.empty()) {
        const char* previewText =
            (state.selectedDeviceIdx >= 0 && std::cmp_less(state.selectedDeviceIdx, validDevices.size()))
                ? validDevices.at(static_cast<size_t>(state.selectedDeviceIdx))->device->GetName().c_str()
                : "Select Device...";
        ImGui::SetNextItemWidth(160.0F);
        if (ImGui::BeginCombo("##DevicePreset", previewText)) {
            for (size_t i = 0; i < validDevices.size(); ++i) {
                bool isSelected = std::cmp_equal(state.selectedDeviceIdx, i);
                if (ImGui::Selectable(validDevices.at(i)->device->GetName().c_str(), isSelected)) {
                    state.selectedDeviceIdx = static_cast<int>(i);
                    state.startAddr = validDevices.at(i)->startAddress;
                    state.endAddr = validDevices.at(i)->endAddress;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::SetNextItemWidth(80.0F);
    uint16_t startVal = state.startAddr;
    if (ImGui::InputScalar("Start", ImGuiDataType_U16, &startVal, nullptr, nullptr, "0x%04X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        state.startAddr = startVal;
        state.selectedDeviceIdx = -1;
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(65.0F);
    ImGui::Combo("##SizeMode", &state.sizeMode, "End\0Size\0");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0F);
    if (state.sizeMode == 0) {
        uint16_t endVal = state.endAddr;
        if (ImGui::InputScalar("##EndAddr", ImGuiDataType_U16, &endVal, nullptr, nullptr, "0x%04X",
                               ImGuiInputTextFlags_CharsHexadecimal)) {
            state.endAddr = endVal;
            state.selectedDeviceIdx = -1;
        }
    } else {
        int length = (state.endAddr >= state.startAddr) ? (state.endAddr - state.startAddr + 1) : 1;
        if (ImGui::DragInt("##Length", &length, 1.0F, 1, 0x10000, "%d B")) {
            length = std::clamp(length, 1, 0x10000);
            int newEnd = static_cast<int>(state.startAddr) + length - 1;
            state.endAddr = static_cast<uint16_t>(std::clamp(newEnd, 0, 0xFFFF));
            state.selectedDeviceIdx = -1;
        }
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("+1B")) {
        if (state.endAddr < 0xFFFF) {
            state.endAddr++;
            state.selectedDeviceIdx = -1;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("+16B")) {
        int newEnd = static_cast<int>(state.endAddr) + 16;
        state.endAddr = static_cast<uint16_t>(std::min(newEnd, 0xFFFF));
        state.selectedDeviceIdx = -1;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("+256B")) {
        int newEnd = static_cast<int>(state.endAddr) + 256;
        state.endAddr = static_cast<uint16_t>(std::min(newEnd, 0xFFFF));
        state.selectedDeviceIdx = -1;
    }

    if (state.startAddr > state.endAddr) {
        std::swap(state.startAddr, state.endAddr);
    }
}

void DrawAsciiStringsView(Hardware::Bus& bus, uint16_t startAddr, uint16_t endAddr) {
    int totalBytes = endAddr - startAddr + 1;

    if (ImGui::CollapsingHeader("Detected Strings in Range")) {
        if (ImGui::BeginChild("DetectedStringsChild", ImVec2(0, 80.0F), ImGuiChildFlags_Borders)) {
            int strStart = -1;
            std::string currentStr;
            int foundCount = 0;

            for (int i = 0; i <= totalBytes; ++i) {
                uint8_t byte = (i < totalBytes) ? bus.ReadDirect(static_cast<Word>(startAddr + i)) : 0;
                if (byte >= 32 && byte <= 126) {
                    if (strStart == -1) {
                        strStart = i;
                    }
                    currentStr.push_back(static_cast<char>(byte));
                } else {
                    if (currentStr.size() >= 3) {
                        RenderText(std::format("0x{:04X} (+{}): \"{}\"", static_cast<Word>(startAddr + strStart),
                                               static_cast<int>(currentStr.size()), currentStr));
                        foundCount++;
                    }
                    currentStr.clear();
                    strStart = -1;
                }
            }
            if (foundCount == 0) {
                RenderTextDisabled("No strings (>= 3 chars) found in selected range.");
            }
        }
        ImGui::EndChild();
    }

    if (ImGui::BeginTable("AsciiByteTable", 4,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
                          ImVec2(0, 200.0F))) {
        ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 60.0F);
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 70.0F);
        ImGui::TableSetupColumn("Hex", ImGuiTableColumnFlags_WidthFixed, 50.0F);
        ImGui::TableSetupColumn("ASCII Char", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(totalBytes);
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                ImGui::TableNextRow();
                auto addr = static_cast<Word>(startAddr + i);
                uint8_t byte = bus.ReadDirect(addr);

                ImGui::TableSetColumnIndex(0);
                RenderTextDisabled(std::format("+0x{:04X}", i));

                ImGui::TableSetColumnIndex(1);
                RenderText(std::format("0x{:04X}", addr));

                ImGui::TableSetColumnIndex(2);
                RenderText(std::format("{:02X}", byte));

                ImGui::TableSetColumnIndex(3);
                if (byte >= 32 && byte <= 126) {
                    RenderText(std::format("'{}'", static_cast<char>(byte)));
                } else if (byte == 0) {
                    RenderTextDisabled("\\0 (NULL)");
                } else {
                    RenderTextDisabled(std::format(". (0x{:02X})", byte));
                }
            }
        }
        ImGui::EndTable();
    }
}

void DrawPointersView(Hardware::Bus& bus, const std::vector<DeviceRegistration>& devices, InspectorState& state) {
    int totalPointers = (state.endAddr - state.startAddr + 1) / 2;
    if (totalPointers <= 0) {
        RenderTextDisabled("Range must be at least 2 bytes to inspect 16-bit pointers.");
        return;
    }

    if (ImGui::BeginTable("PointersTable", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
                          ImVec2(0, 220.0F))) {
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 65.0F);
        ImGui::TableSetupColumn("Bytes [L H]", ImGuiTableColumnFlags_WidthFixed, 85.0F);
        ImGui::TableSetupColumn("Target Pointer", ImGuiTableColumnFlags_WidthFixed, 95.0F);
        ImGui::TableSetupColumn("Target Device", ImGuiTableColumnFlags_WidthFixed, 95.0F);
        ImGui::TableSetupColumn("*Target Val", ImGuiTableColumnFlags_WidthFixed, 85.0F);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 65.0F);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(totalPointers);
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                ImGui::PushID(i);
                ImGui::TableNextRow();
                auto addr = static_cast<Word>(state.startAddr + (i * 2));
                uint8_t low = bus.ReadDirect(addr);
                uint8_t high = bus.ReadDirect(static_cast<Word>(addr + 1));
                auto targetPtr = static_cast<Word>(low | (high << 8));

                ImGui::TableSetColumnIndex(0);
                RenderText(std::format("0x{:04X}", addr));

                ImGui::TableSetColumnIndex(1);
                RenderText(std::format("{:02X} {:02X}", low, high));

                ImGui::TableSetColumnIndex(2);
                RenderText(std::format("0x{:04X}", targetPtr));

                ImGui::TableSetColumnIndex(3);
                const DeviceRegistration* hitDev = nullptr;
                for (const auto& reg : devices) {
                    if (reg.enabled && targetPtr >= reg.startAddress && targetPtr <= reg.endAddress) {
                        hitDev = &reg;
                        break;
                    }
                }
                if (hitDev != nullptr) {
                    ImGui::PushStyleColor(ImGuiCol_Text, GetDeviceColor(hitDev->device->GetName()));
                    ImGui::TextUnformatted(hitDev->device->GetName().c_str());
                    ImGui::PopStyleColor();
                } else {
                    RenderTextDisabled("Unmapped");
                }

                ImGui::TableSetColumnIndex(4);
                uint8_t derefVal = bus.ReadDirect(targetPtr);
                char derefChar = (derefVal >= 32 && derefVal <= 126) ? static_cast<char>(derefVal) : '.';
                RenderText(std::format("0x{:02X} ('{}')", derefVal, derefChar));

                ImGui::TableSetColumnIndex(5);
                if (ImGui::SmallButton("Inspect")) {
                    state.startAddr = targetPtr;
                    state.endAddr = static_cast<uint16_t>(std::min(static_cast<int>(targetPtr) + 31, 0xFFFF));
                    state.selectedDeviceIdx = -1;
                }

                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
}

void DrawSignedIntegersView(Hardware::Bus& bus, uint16_t startAddr, uint16_t endAddr, int& mode) {
    ImGui::RadioButton("8-bit (int8_t)", &mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("16-bit Little-Endian (int16_t)", &mode, 1);

    int totalItems = (mode == 0) ? (endAddr - startAddr + 1) : ((endAddr - startAddr + 1) / 2);
    if (totalItems <= 0) {
        RenderTextDisabled("Not enough bytes in range for selected integer width.");
        return;
    }

    std::ostringstream snippet;
    snippet << "{ ";
    int previewCount = std::min(totalItems, 16);
    for (int i = 0; i < previewCount; ++i) {
        if (i > 0) {
            snippet << ", ";
        }
        if (mode == 0) {
            auto addr = static_cast<Word>(startAddr + i);
            auto val = static_cast<int8_t>(bus.ReadDirect(addr));
            snippet << static_cast<int>(val);
        } else {
            auto addr = static_cast<Word>(startAddr + (i * 2));
            uint8_t low = bus.ReadDirect(addr);
            uint8_t high = bus.ReadDirect(static_cast<Word>(addr + 1));
            auto val = static_cast<int16_t>(low | (high << 8));
            snippet << static_cast<int>(val);
        }
    }
    if (totalItems > previewCount) {
        snippet << ", ...";
    }
    snippet << " }";

    RenderTextDisabled("Array preview:");
    ImGui::SameLine();
    ImGui::TextUnformatted(snippet.str().c_str());

    if (ImGui::BeginTable("SignedIntTable", (mode == 0) ? 5 : 4,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
                          ImVec2(0, 200.0F))) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50.0F);
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 70.0F);
        ImGui::TableSetupColumn("Signed (Dec)", ImGuiTableColumnFlags_WidthFixed, 90.0F);
        ImGui::TableSetupColumn("Hex", ImGuiTableColumnFlags_WidthFixed, 70.0F);
        if (mode == 0) {
            ImGui::TableSetupColumn("Unsigned (Dec)", ImGuiTableColumnFlags_WidthStretch);
        }
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(totalItems);
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                RenderText(std::format("[{}]", i));

                if (mode == 0) {
                    auto addr = static_cast<Word>(startAddr + i);
                    uint8_t raw = bus.ReadDirect(addr);
                    auto sVal = static_cast<int8_t>(raw);

                    ImGui::TableSetColumnIndex(1);
                    RenderText(std::format("0x{:04X}", addr));

                    ImGui::TableSetColumnIndex(2);
                    RenderText(std::format("{}", static_cast<int>(sVal)));

                    ImGui::TableSetColumnIndex(3);
                    RenderText(std::format("0x{:02X}", raw));

                    ImGui::TableSetColumnIndex(4);
                    RenderText(std::format("{}", raw));
                } else {
                    auto addr = static_cast<Word>(startAddr + (i * 2));
                    uint8_t low = bus.ReadDirect(addr);
                    uint8_t high = bus.ReadDirect(static_cast<Word>(addr + 1));
                    auto raw = static_cast<uint16_t>(low | (high << 8));
                    auto sVal = static_cast<int16_t>(raw);

                    ImGui::TableSetColumnIndex(1);
                    RenderText(std::format("0x{:04X}", addr));

                    ImGui::TableSetColumnIndex(2);
                    RenderText(std::format("{}", static_cast<int>(sVal)));

                    ImGui::TableSetColumnIndex(3);
                    RenderText(std::format("0x{:04X}", raw));
                }
            }
        }
        ImGui::EndTable();
    }
}

void DrawDataInspector(Hardware::Bus& bus, const std::vector<DeviceRegistration>& devices, InspectorState& state) {
    ImGui::TextUnformatted("Data & Structure Inspector");
    ImGui::Separator();

    DrawInspectorRangeControls(devices, state);
    ImGui::Spacing();

    if (ImGui::BeginTabBar("InspectorTabs")) {
        if (ImGui::BeginTabItem("ASCII Strings")) {
            DrawAsciiStringsView(bus, state.startAddr, state.endAddr);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("16-bit Pointers")) {
            DrawPointersView(bus, devices, state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Signed Integers")) {
            DrawSignedIntegersView(bus, state.startAddr, state.endAddr, state.signedIntMode);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

}  // namespace

void DrawMemoryLayoutWindow(AppState& state) {
    Bus& bus = state.emulator.GetMem();
    static InspectorState inspector;
    static std::vector<DeviceRegistration> editDevices;
    static bool layoutDirty = true;
    if (editDevices.size() != bus.GetRegisteredDevices().size()) {
        editDevices = bus.GetRegisteredDevices();
        layoutDirty = true;
    }

    static std::array<unsigned char, 256ULL * 256ULL * 3ULL> pixels{};
    if (layoutDirty) {
        UpdateMemoryLayoutTexture(state, editDevices, pixels);
        layoutDirty = false;
    }

    ImGui::Columns(2, "LayoutColumns", true);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.4F);

    ImGui::TextUnformatted("Memory Map Visualization");
    ImGui::Separator();
    float mapSize = ImGui::GetContentRegionAvail().x;
    mapSize = std::min(mapSize, ImGui::GetContentRegionAvail().y - 40.0F);

    ImGui::Image((ImTextureID)(intptr_t)state.render.layoutTexture,
                 ImVec2(mapSize, mapSize));

    if (ImGui::IsItemHovered()) {
        DrawMemoryLayoutTooltip(editDevices, mapSize, mapSize);
    }

    ImVec2 imgPos = ImGui::GetItemRectMin();
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        int memY = std::clamp(static_cast<int>((mousePos.y - imgPos.y) / mapSize * 256.0F), 0, 255);
        inspector.startAddr = static_cast<uint16_t>(memY << 8);
        inspector.endAddr = static_cast<uint16_t>((memY << 8) | 0xFF);
        inspector.selectedDeviceIdx = -1;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float scale = mapSize / 256.0F;
    int pageStart = inspector.startAddr >> 8;
    int pageEnd = inspector.endAddr >> 8;
    if (pageStart == pageEnd) {
        int x0 = inspector.startAddr & 0xFF;
        int x1 = inspector.endAddr & 0xFF;
        ImVec2 p0(imgPos.x + (static_cast<float>(x0) * scale), imgPos.y + (static_cast<float>(pageStart) * scale));
        ImVec2 p1(imgPos.x + (static_cast<float>(x1 + 1) * scale),
                  imgPos.y + (static_cast<float>(pageStart + 1) * scale));
        drawList->AddRectFilled(p0, p1, IM_COL32(255, 230, 0, 110));
        drawList->AddRect(p0, p1, IM_COL32(255, 255, 0, 255));
    } else {
        ImVec2 p0(imgPos.x, imgPos.y + (static_cast<float>(pageStart) * scale));
        ImVec2 p1(imgPos.x + mapSize, imgPos.y + (static_cast<float>(pageEnd + 1) * scale));
        drawList->AddRectFilled(p0, p1, IM_COL32(255, 230, 0, 70));
        drawList->AddRect(p0, p1, IM_COL32(255, 255, 0, 220));
    }

    ImGui::NextColumn();

    DrawMappedDevicesTable(state, editDevices, layoutDirty);

    ImGui::Spacing();
    ImGui::Separator();

    static std::array<char, 256> collisionError{};
    static bool showCollisionError = false;

    bool isCartridgeLoaded = state.emulator.GetCartridge().loaded;
    if (isCartridgeLoaded) {
        ImGui::BeginDisabled(true);
    }

    if (ImGui::Button("Apply & Reset Emulator", ImVec2(-1, 40))) {
        bool wasRunning = !state.emulator.IsPaused();
        if (wasRunning) {
            state.emulator.Pause();
        }

        try {
            state.emulator.WaitUntilSafeToMutate();
            bus.GetRegisteredDevices() = editDevices;
            bus.RebuildDeviceMap();
            state.emulator.GetCPU().Reset();
            state.emulator.GetCPU().PC = static_cast<uint16_t>(bus.ReadDirect(0xFFFC) | (bus.ReadDirect(0xFFFD) << 8));
            state.emulator.GetCPU().UpdatePagePtr(bus);
            if (wasRunning) {
                state.emulator.Resume();
            }
        } catch (const std::runtime_error& e) {
            strncpy(collisionError.data(), e.what(), 255);
            showCollisionError = true;
        }
    }

    if (isCartridgeLoaded) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (ImGui::BeginTooltip()) {
                ImGui::TextUnformatted("Hardware mapping cannot be modified when a cartridge is loaded.");
                ImGui::EndTooltip();
            }
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

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    DrawDataInspector(bus, editDevices, inspector);

    ImGui::Columns(1);
}

}  // namespace GUI
