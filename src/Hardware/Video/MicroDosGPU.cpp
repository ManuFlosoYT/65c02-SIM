#include "Hardware/Video/MicroDosGPU.h"

#include <algorithm>

namespace Hardware {

void MicroDosGPU::Reset() {
    GPU::Reset();
    extraRam.fill(0);
    mouse.Reset();
    burstStep = 0U;
    fpgaAddr = 0U;
    fpgaData = 0U;
    fpgaRw = true;
    hostTargetX.store(0U, std::memory_order_relaxed);
    hostTargetY.store(0U, std::memory_order_relaxed);
    hostTargetButtons.store(0U, std::memory_order_relaxed);
    hostMousePending.store(false, std::memory_order_relaxed);
}

void MicroDosGPU::UpdateHostMouse(Byte targetX, Byte targetY, Byte buttons) noexcept {
    Byte clampedX = std::min(targetX, PS2MouseController::MAX_X);
    Byte clampedY = std::min(targetY, PS2MouseController::MAX_Y);
    hostTargetX.store(clampedX, std::memory_order_relaxed);
    hostTargetY.store(clampedY, std::memory_order_relaxed);
    hostTargetButtons.store(static_cast<Byte>(buttons & 0x07U), std::memory_order_relaxed);
    hostMousePending.store(true, std::memory_order_release);
}

void MicroDosGPU::Clock() {
    GPU::Clock();
    mouse.Clock();

    if (mouse.IsStreamMode() && hostMousePending.exchange(false, std::memory_order_acquire)) {
        Byte tx = hostTargetX.load(std::memory_order_relaxed);
        Byte ty = hostTargetY.load(std::memory_order_relaxed);
        Byte tb = hostTargetButtons.load(std::memory_order_relaxed);
        auto dx = static_cast<int16_t>(static_cast<int16_t>(tx) - static_cast<int16_t>(mouse.GetPosX()));
        auto dyPs2 = static_cast<int16_t>(static_cast<int16_t>(mouse.GetPosY()) - static_cast<int16_t>(ty));
        if (dx != 0 || dyPs2 != 0 || tb != mouse.GetButtons()) {
            mouse.InjectPacket(dx, dyPs2, tb);
        }
    }

    if (!IsInBlankingInterval()) {
        fpgaRw = true;
        return;
    }

    if (mouse.IsDirty() || burstStep > 0U) {
        if (burstStep == 0U) {
            fpgaAddr = MOUSE_BTNS_OFFSET;
            fpgaData = mouse.GetButtons();
            fpgaRw = false;
            extraRam.at(MOUSE_BTNS_OFFSET) = fpgaData;
            burstStep = 1U;
        } else if (burstStep == 1U) {
            fpgaAddr = MOUSE_X_OFFSET;
            fpgaData = mouse.GetPosX();
            fpgaRw = false;
            extraRam.at(MOUSE_X_OFFSET) = fpgaData;
            burstStep = 2U;
        } else {
            fpgaAddr = MOUSE_Y_OFFSET;
            fpgaData = mouse.GetPosY();
            fpgaRw = false;
            extraRam.at(MOUSE_Y_OFFSET) = fpgaData;
            burstStep = 0U;
            mouse.ClearDirty();
        }
    } else {
        fpgaRw = true;
    }
}

void MicroDosGPU::LoadVRAM(std::span<const uint8_t> data) {
    GPU::LoadVRAM(data);
    size_t limit = std::min(data.size(), MEMORY_SIZE);
    for (size_t i = 0; i < limit; ++i) {
        Byte pixelXAddr = static_cast<Byte>(i & 0x7F);
        Byte pixelYAddr = static_cast<Byte>((i >> 7) & 0x7F);
        if (pixelXAddr >= VRAM_WIDTH || pixelYAddr >= VRAM_HEIGHT) {
            extraRam[i] = data[i];
        }
    }
}

bool MicroDosGPU::SaveState(std::ostream& out) const {
    if (!GPU::SaveState(out)) {
        return false;
    }
    ISerializable::Serialize(out, extraRam);
    ISerializable::Serialize(out, mouse.fpgaClkOut);
    ISerializable::Serialize(out, mouse.fpgaDataOut);
    ISerializable::Serialize(out, mouse.mouseClkOut);
    ISerializable::Serialize(out, mouse.mouseDataOut);
    ISerializable::Serialize(out, mouse.prevPs2Clk);
    auto linkStateVal = static_cast<uint8_t>(mouse.linkState);
    ISerializable::Serialize(out, linkStateVal);
    ISerializable::Serialize(out, mouse.stateCounter);
    ISerializable::Serialize(out, mouse.shiftReg);
    ISerializable::Serialize(out, mouse.bitIndex);
    ISerializable::Serialize(out, mouse.lastRxByte);
    ISerializable::Serialize(out, mouse.lastHostTxByte);
    ISerializable::Serialize(out, mouse.packet);
    ISerializable::Serialize(out, mouse.packetIndex);
    ISerializable::Serialize(out, mouse.posX);
    ISerializable::Serialize(out, mouse.posY);
    ISerializable::Serialize(out, mouse.buttons);
    ISerializable::Serialize(out, mouse.mouseDirtyFlag);
    ISerializable::Serialize(out, burstStep);
    ISerializable::Serialize(out, fpgaAddr);
    ISerializable::Serialize(out, fpgaData);
    ISerializable::Serialize(out, fpgaRw);
    return out.good();
}

bool MicroDosGPU::LoadState(std::istream& inputStream) {
    if (!GPU::LoadState(inputStream)) {
        return false;
    }
    ISerializable::Deserialize(inputStream, extraRam);
    ISerializable::Deserialize(inputStream, mouse.fpgaClkOut);
    ISerializable::Deserialize(inputStream, mouse.fpgaDataOut);
    ISerializable::Deserialize(inputStream, mouse.mouseClkOut);
    ISerializable::Deserialize(inputStream, mouse.mouseDataOut);
    ISerializable::Deserialize(inputStream, mouse.prevPs2Clk);
    uint8_t linkStateVal = 0U;
    ISerializable::Deserialize(inputStream, linkStateVal);
    mouse.linkState = static_cast<PS2LinkState>(linkStateVal);
    ISerializable::Deserialize(inputStream, mouse.stateCounter);
    ISerializable::Deserialize(inputStream, mouse.shiftReg);
    ISerializable::Deserialize(inputStream, mouse.bitIndex);
    ISerializable::Deserialize(inputStream, mouse.lastRxByte);
    ISerializable::Deserialize(inputStream, mouse.lastHostTxByte);
    ISerializable::Deserialize(inputStream, mouse.packet);
    ISerializable::Deserialize(inputStream, mouse.packetIndex);
    ISerializable::Deserialize(inputStream, mouse.posX);
    ISerializable::Deserialize(inputStream, mouse.posY);
    ISerializable::Deserialize(inputStream, mouse.buttons);
    ISerializable::Deserialize(inputStream, mouse.mouseDirtyFlag);
    ISerializable::Deserialize(inputStream, burstStep);
    ISerializable::Deserialize(inputStream, fpgaAddr);
    ISerializable::Deserialize(inputStream, fpgaData);
    ISerializable::Deserialize(inputStream, fpgaRw);
    hostMousePending.store(false, std::memory_order_relaxed);
    return inputStream.good();
}

}  // namespace Hardware

