#include "Hardware/Audio/NSFRunner.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

namespace {

constexpr std::array<uint8_t, 32> kLengthTable = {10, 254, 20,  2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
                                                  12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

constexpr uint16_t kSentinelAddr = 0x5FF0U;

struct ParsedNSFImage {
    bool valid = false;
    std::string error;
    uint8_t version = 1;
    uint8_t totalSongs = 1;
    uint8_t startingSong = 1;
    uint16_t loadAddr = 0x8000;
    uint16_t initAddr = 0x8000;
    uint16_t playAddr = 0x8003;
    std::string songName;
    std::string artist;
    std::string copyright;
    uint16_t playSpeedNtsc = 16639;
    uint16_t playSpeedPal = 19997;
    uint8_t palNtscBits = 0;
    std::array<uint8_t, 8> bankswitchInit{};
    bool hasExplicitBanks = false;
    double metaDurationSec = 0.0;
    std::span<const uint8_t> payload;
};

std::string ReadFixedCString(std::span<const uint8_t> bytes) {
    std::string out;
    out.reserve(bytes.size());
    for (uint8_t b : bytes) {
        if (b == 0U) {
            break;
        }
        out.push_back(static_cast<char>(b));
    }
    return out;
}

std::string EscapeJsonString(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 2U);
    out.push_back('"');
    for (unsigned char ch : input) {
        switch (ch) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (ch < 0x20U || ch >= 0x7FU) {
                    constexpr std::string_view kHex = "0123456789abcdef";
                    out += "\\u00";
                    out.push_back(kHex[(ch >> 4U) & 0x0FU]);
                    out.push_back(kHex[ch & 0x0FU]);
                } else {
                    out.push_back(static_cast<char>(ch));
                }
                break;
        }
    }
    out.push_back('"');
    return out;
}

uint16_t ReadLE16(std::span<const uint8_t> data, size_t offset) {
    return static_cast<uint16_t>(data[offset]) | (static_cast<uint16_t>(data[offset + 1U]) << 8U);
}

uint32_t ReadLE32(std::span<const uint8_t> data, size_t offset) {
    return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1U]) << 8U) |
           (static_cast<uint32_t>(data[offset + 2U]) << 16U) | (static_cast<uint32_t>(data[offset + 3U]) << 24U);
}

void ParseNSFeChunks(std::span<const uint8_t> chunkStream, int targetSongZeroBased, ParsedNSFImage& out,
                     bool allowDataAndInfo) {
    size_t pos = 0;
    while (pos + 8U <= chunkStream.size()) {
        const uint32_t chunkSize = ReadLE32(chunkStream, pos);
        const std::string_view fourCC(reinterpret_cast<const char*>(chunkStream.data() + pos + 4U), 4U);
        pos += 8U;
        if (pos + chunkSize > chunkStream.size()) {
            break;
        }
        const std::span<const uint8_t> chunk = chunkStream.subspan(pos, chunkSize);
        pos += chunkSize;

        if (fourCC == "NEND") {
            break;
        }
        if (allowDataAndInfo && fourCC == "INFO" && chunk.size() >= 9U) {
            out.loadAddr = ReadLE16(chunk, 0U);
            out.initAddr = ReadLE16(chunk, 2U);
            out.playAddr = ReadLE16(chunk, 4U);
            out.palNtscBits = chunk[6U];
            out.totalSongs = std::max<uint8_t>(1U, chunk[8U]);
            out.startingSong = (chunk.size() >= 10U) ? static_cast<uint8_t>(chunk[9U] + 1U) : 1U;
        } else if (allowDataAndInfo && fourCC == "DATA") {
            out.payload = chunk;
        } else if (allowDataAndInfo && fourCC == "BANK") {
            out.hasExplicitBanks = true;
            for (size_t i = 0; i < std::min<size_t>(8U, chunk.size()); ++i) {
                out.bankswitchInit[i] = chunk[i];
            }
        } else if (fourCC == "time") {
            const size_t idx = static_cast<size_t>(std::max(0, targetSongZeroBased));
            if ((idx + 1U) * 4U <= chunk.size()) {
                const auto ms = static_cast<int32_t>(ReadLE32(chunk, idx * 4U));
                if (ms > 0) {
                    out.metaDurationSec = static_cast<double>(ms) / 1000.0;
                }
            }
        } else if (fourCC == "auth") {
            size_t strPos = 0;
            auto nextStr = [&]() -> std::string {
                std::string s;
                while (strPos < chunk.size() && chunk[strPos] != 0U) {
                    s.push_back(static_cast<char>(chunk[strPos++]));
                }
                if (strPos < chunk.size() && chunk[strPos] == 0U) {
                    ++strPos;
                }
                return s;
            };
            if (strPos < chunk.size()) {
                out.songName = nextStr();
            }
            if (strPos < chunk.size()) {
                out.artist = nextStr();
            }
            if (strPos < chunk.size()) {
                out.copyright = nextStr();
            }
        }
    }
}

ParsedNSFImage ParseNSF(std::span<const uint8_t> nsfData, int requestedSongIndex) {
    ParsedNSFImage img;
    if (nsfData.size() >= 4U && nsfData[0] == 'N' && nsfData[1] == 'S' && nsfData[2] == 'F' && nsfData[3] == 'E') {
        const int songZero = (requestedSongIndex > 0) ? (requestedSongIndex - 1) : 0;
        ParseNSFeChunks(nsfData.subspan(4U), songZero, img, true);
        if (img.payload.empty()) {
            img.error = "NSFE image missing DATA chunk";
            return img;
        }
        if (img.loadAddr < 0x6000U) {
            img.error = "NSF load address below $6000 is unsupported";
            return img;
        }
        img.valid = true;
        return img;
    }

    if (nsfData.size() < 128U || nsfData[0] != 'N' || nsfData[1] != 'E' || nsfData[2] != 'S' || nsfData[3] != 'M' ||
        nsfData[4] != 0x1AU) {
        img.error = "Invalid NSF header magic or file too small (< 128 bytes)";
        return img;
    }

    img.version = nsfData[5];
    img.totalSongs = std::max<uint8_t>(1U, nsfData[6]);
    img.startingSong = std::clamp<uint8_t>(nsfData[7], 1U, img.totalSongs);
    img.loadAddr = ReadLE16(nsfData, 8U);
    img.initAddr = ReadLE16(nsfData, 10U);
    img.playAddr = ReadLE16(nsfData, 12U);
    img.songName = ReadFixedCString(nsfData.subspan(0x0EU, 32U));
    img.artist = ReadFixedCString(nsfData.subspan(0x2EU, 32U));
    img.copyright = ReadFixedCString(nsfData.subspan(0x4EU, 32U));
    img.playSpeedNtsc = ReadLE16(nsfData, 0x6EU);
    for (size_t i = 0; i < 8U; ++i) {
        img.bankswitchInit[i] = nsfData[0x70U + i];
    }
    img.hasExplicitBanks =
        std::any_of(img.bankswitchInit.begin(), img.bankswitchInit.end(), [](uint8_t b) { return b != 0U; });
    img.playSpeedPal = ReadLE16(nsfData, 0x78U);
    img.palNtscBits = nsfData[0x7AU];

    if (img.loadAddr < 0x6000U) {
        img.error = "NSF load address below $6000 is unsupported";
        return img;
    }

    img.payload = nsfData.subspan(128U);
    if (img.version == 2U) {
        const uint32_t prgSize = static_cast<uint32_t>(nsfData[0x7DU]) |
                                 (static_cast<uint32_t>(nsfData[0x7EU]) << 8U) |
                                 (static_cast<uint32_t>(nsfData[0x7FU]) << 16U);
        if (prgSize > 0U && 128U + prgSize <= nsfData.size()) {
            img.payload = nsfData.subspan(128U, prgSize);
            const int songZero = (requestedSongIndex > 0) ? (requestedSongIndex - 1)
                                                          : (static_cast<int>(img.startingSong) - 1);
            ParseNSFeChunks(nsfData.subspan(128U + prgSize), songZero, img, false);
        }
    }

    if (img.payload.empty()) {
        img.error = "NSF payload is empty";
        return img;
    }

    img.valid = true;
    return img;
}

struct PulseChannelState {
    uint8_t duty = 0;
    bool lengthHalt = false;
    bool constantVolume = false;
    uint8_t volumeOrPeriod = 0;
    bool sweepEnabled = false;
    uint8_t sweepPeriod = 0;
    bool sweepNegate = true;
    uint8_t sweepShift = 0;
    bool sweepReload = false;
    uint8_t sweepDivider = 0;
    uint16_t timer = 0;
    uint8_t lengthCounter = 0;
    bool envStart = false;
    uint8_t envDecayLevel = 0;
    uint8_t envDivider = 0;
    bool enabled = true;
    bool wroteHighByte = false;

    [[nodiscard]] bool IsSweepMuted() const {
        if (timer < 8U) {
            return true;
        }
        if (sweepEnabled && sweepShift > 0U && !sweepNegate) {
            const uint16_t change = timer >> sweepShift;
            if (timer + change > 0x07FFU) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] uint8_t EffectiveVolume() const {
        if (!enabled || lengthCounter == 0U || IsSweepMuted()) {
            return 0U;
        }
        return constantVolume ? volumeOrPeriod : envDecayLevel;
    }
};

struct TriangleChannelState {
    bool controlFlag = false;
    uint8_t linearReloadValue = 0;
    uint8_t linearCounter = 0;
    bool linearReload = false;
    uint16_t timer = 0;
    uint8_t lengthCounter = 0;
    bool enabled = true;
    bool wroteHighByte = false;

    [[nodiscard]] uint8_t EffectiveVolume() const {
        if (!enabled || lengthCounter == 0U || linearCounter == 0U || timer < 2U || timer > 0x07FFU) {
            return 0U;
        }
        return 15U;
    }
};

struct NoiseChannelState {
    bool lengthHalt = false;
    bool constantVolume = false;
    uint8_t volumeOrPeriod = 0;
    uint8_t mode = 0;
    uint8_t periodIdx = 0;
    uint8_t lengthCounter = 0;
    bool envStart = false;
    uint8_t envDecayLevel = 0;
    uint8_t envDivider = 0;
    bool enabled = true;
    bool wroteHighByte = false;

    [[nodiscard]] uint8_t EffectiveVolume() const {
        if (!enabled || lengthCounter == 0U) {
            return 0U;
        }
        return constantVolume ? volumeOrPeriod : envDecayLevel;
    }
};

class SimulatedAPU {
public:
    std::array<PulseChannelState, 2> pulse{};
    TriangleChannelState triangle{};
    NoiseChannelState noise{};
    bool wroteAnyRegisterThisFrame = false;

    void Reset() {
        pulse = {};
        triangle = {};
        noise = {};
        wroteAnyRegisterThisFrame = false;
    }

    void BeginFrame() {
        wroteAnyRegisterThisFrame = false;
        pulse[0].wroteHighByte = false;
        pulse[1].wroteHighByte = false;
        triangle.wroteHighByte = false;
        noise.wroteHighByte = false;
    }

    [[nodiscard]] uint8_t ReadStatus() const {
        uint8_t status = 0U;
        if (pulse[0].lengthCounter > 0U) {
            status |= 0x01U;
        }
        if (pulse[1].lengthCounter > 0U) {
            status |= 0x02U;
        }
        if (triangle.lengthCounter > 0U) {
            status |= 0x04U;
        }
        if (noise.lengthCounter > 0U) {
            status |= 0x08U;
        }
        return status;
    }

    void WriteRegister(uint16_t addr, uint8_t val) {
        wroteAnyRegisterThisFrame = true;
        switch (addr) {
            case 0x4000U:
            case 0x4004U: {
                auto& p = pulse[(addr == 0x4000U) ? 0U : 1U];
                p.duty = (val >> 6U) & 0x03U;
                p.lengthHalt = (val & 0x20U) != 0U;
                p.constantVolume = (val & 0x10U) != 0U;
                p.volumeOrPeriod = val & 0x0FU;
                break;
            }
            case 0x4001U:
            case 0x4005U: {
                auto& p = pulse[(addr == 0x4001U) ? 0U : 1U];
                p.sweepEnabled = (val & 0x80U) != 0U;
                p.sweepPeriod = (val >> 4U) & 0x07U;
                p.sweepNegate = (val & 0x08U) != 0U;
                p.sweepShift = val & 0x07U;
                p.sweepReload = true;
                break;
            }
            case 0x4002U:
            case 0x4006U: {
                auto& p = pulse[(addr == 0x4002U) ? 0U : 1U];
                p.timer = static_cast<uint16_t>((p.timer & 0x0700U) | val);
                break;
            }
            case 0x4003U:
            case 0x4007U: {
                auto& p = pulse[(addr == 0x4003U) ? 0U : 1U];
                p.timer = static_cast<uint16_t>((p.timer & 0x00FFU) | (static_cast<uint16_t>(val & 0x07U) << 8U));
                if (p.enabled) {
                    p.lengthCounter = kLengthTable[(val >> 3U) & 0x1FU];
                }
                p.envDecayLevel = 15U;
                p.envDivider = p.volumeOrPeriod;
                p.envStart = false;
                p.wroteHighByte = true;
                break;
            }
            case 0x4008U:
                triangle.controlFlag = (val & 0x80U) != 0U;
                triangle.linearReloadValue = val & 0x7FU;
                if (triangle.linearReload) {
                    triangle.linearCounter = triangle.linearReloadValue;
                }
                break;
            case 0x400AU:
                triangle.timer = static_cast<uint16_t>((triangle.timer & 0x0700U) | val);
                break;
            case 0x400BU:
                triangle.timer =
                    static_cast<uint16_t>((triangle.timer & 0x00FFU) | (static_cast<uint16_t>(val & 0x07U) << 8U));
                if (triangle.enabled) {
                    triangle.lengthCounter = kLengthTable[(val >> 3U) & 0x1FU];
                }
                triangle.linearReload = true;
                triangle.linearCounter = triangle.linearReloadValue;
                triangle.wroteHighByte = true;
                break;
            case 0x400CU:
                noise.lengthHalt = (val & 0x20U) != 0U;
                noise.constantVolume = (val & 0x10U) != 0U;
                noise.volumeOrPeriod = val & 0x0FU;
                break;
            case 0x400EU:
                noise.mode = (val >> 7U) & 0x01U;
                noise.periodIdx = val & 0x0FU;
                break;
            case 0x400FU:
                if (noise.enabled) {
                    noise.lengthCounter = kLengthTable[(val >> 3U) & 0x1FU];
                }
                noise.envDecayLevel = 15U;
                noise.envDivider = noise.volumeOrPeriod;
                noise.envStart = false;
                noise.wroteHighByte = true;
                break;
            case 0x4015U:
                pulse[0].enabled = (val & 0x01U) != 0U;
                if (!pulse[0].enabled) {
                    pulse[0].lengthCounter = 0U;
                }
                pulse[1].enabled = (val & 0x02U) != 0U;
                if (!pulse[1].enabled) {
                    pulse[1].lengthCounter = 0U;
                }
                triangle.enabled = (val & 0x04U) != 0U;
                if (!triangle.enabled) {
                    triangle.lengthCounter = 0U;
                }
                noise.enabled = (val & 0x08U) != 0U;
                if (!noise.enabled) {
                    noise.lengthCounter = 0U;
                }
                break;
            default:
                break;
        }
    }

    [[nodiscard]] std::array<NSFChannelFrameState, 4> EvaluateFrameState(
        const std::array<NSFChannelFrameState, 4>& prev, bool isFirstFrame) const {
        std::array<NSFChannelFrameState, 4> out{};

        for (size_t i = 0; i < 2U; ++i) {
            const auto& p = pulse[i];
            const uint8_t vol = p.EffectiveVolume();
            if (vol > 0U) {
                bool retrig = p.wroteHighByte;
                if (retrig && !isFirstFrame && prev[i].volume > 0U) {
                    // Vibrato/slide crossing a 256-period boundary must write $4003/$4007 on 2A03
                    const bool crossedHighPage = ((p.timer ^ prev[i].period) & 0x0700U) != 0U;
                    const int diff = std::abs(static_cast<int>(p.timer) - static_cast<int>(prev[i].period));
                    if (crossedHighPage && diff <= 12) {
                        retrig = false;
                    }
                }
                out[i] = NSFChannelFrameState{
                    .volume = vol,
                    .period = p.timer,
                    .duty = p.duty,
                    .retrigger = retrig,
                };
            }
        }

        const uint8_t triVol = triangle.EffectiveVolume();
        if (triVol > 0U) {
            bool retrig = triangle.wroteHighByte;
            if (retrig && !isFirstFrame && prev[2].volume > 0U) {
                const bool crossedHighPage = ((triangle.timer ^ prev[2].period) & 0x0700U) != 0U;
                const int diff = std::abs(static_cast<int>(triangle.timer) - static_cast<int>(prev[2].period));
                if (crossedHighPage && diff <= 12) {
                    retrig = false;
                }
            }
            out[2] = NSFChannelFrameState{
                .volume = triVol,
                .period = triangle.timer,
                .duty = 0U,
                .retrigger = retrig,
            };
        }

        const uint8_t noiseVol = noise.EffectiveVolume();
        if (noiseVol > 0U) {
            out[3] = NSFChannelFrameState{
                .volume = noiseVol,
                .period = noise.periodIdx,
                .duty = noise.mode,
                .retrigger = noise.wroteHighByte,
            };
        }

        return out;
    }

    void StepFrameSequencer() {
        for (int subStep = 0; subStep < 4; ++subStep) {
            for (auto& p : pulse) {
                if (p.envStart) {
                    p.envStart = false;
                    p.envDecayLevel = 15U;
                    p.envDivider = p.volumeOrPeriod;
                } else if (p.envDivider == 0U) {
                    p.envDivider = p.volumeOrPeriod;
                    if (p.envDecayLevel > 0U) {
                        --p.envDecayLevel;
                    } else if (p.lengthHalt) {
                        p.envDecayLevel = 15U;
                    }
                } else {
                    --p.envDivider;
                }
            }

            if (noise.envStart) {
                noise.envStart = false;
                noise.envDecayLevel = 15U;
                noise.envDivider = noise.volumeOrPeriod;
            } else if (noise.envDivider == 0U) {
                noise.envDivider = noise.volumeOrPeriod;
                if (noise.envDecayLevel > 0U) {
                    --noise.envDecayLevel;
                } else if (noise.lengthHalt) {
                    noise.envDecayLevel = 15U;
                }
            } else {
                --noise.envDivider;
            }

            if (triangle.linearReload) {
                triangle.linearCounter = triangle.linearReloadValue;
            } else if (triangle.linearCounter > 0U) {
                --triangle.linearCounter;
            }
            if (!triangle.controlFlag) {
                triangle.linearReload = false;
            }

            if (subStep == 1 || subStep == 3) {
                for (size_t i = 0; i < 2U; ++i) {
                    auto& p = pulse[i];
                    if (!p.lengthHalt && p.lengthCounter > 0U) {
                        --p.lengthCounter;
                    }
                    if (p.sweepDivider == 0U && p.sweepEnabled && p.sweepShift > 0U && !p.IsSweepMuted()) {
                        const uint16_t change = p.timer >> p.sweepShift;
                        if (p.sweepNegate) {
                            const uint16_t sub = change + ((i == 0U) ? 1U : 0U);
                            p.timer = (p.timer > sub) ? static_cast<uint16_t>(p.timer - sub) : 0U;
                        } else {
                            p.timer = static_cast<uint16_t>(p.timer + change);
                        }
                    }
                    if (p.sweepDivider == 0U || p.sweepReload) {
                        p.sweepDivider = p.sweepPeriod;
                        p.sweepReload = false;
                    } else {
                        --p.sweepDivider;
                    }
                }

                if (!triangle.controlFlag && triangle.lengthCounter > 0U) {
                    --triangle.lengthCounter;
                }
                if (!noise.lengthHalt && noise.lengthCounter > 0U) {
                    --noise.lengthCounter;
                }
            }
        }
    }
};

class NSFBusDevice final : public IBusDevice {
public:
    std::array<uint8_t, 0x0800> nesRam{};
    std::array<uint8_t, 0x2000> wram{};
    std::array<uint8_t, 0x10000> flatMemory{};
    std::vector<std::array<uint8_t, 0x1000>> banks;
    std::array<int32_t, 2> wramBanks{-1, -1};
    std::array<int32_t, 8> romBanks{-1, -1, -1, -1, -1, -1, -1, -1};
    SimulatedAPU apu;

    void LoadImage(const ParsedNSFImage& img) {
        nesRam.fill(0U);
        wram.fill(0U);
        flatMemory.fill(0U);
        wramBanks = {-1, -1};
        romBanks = {-1, -1, -1, -1, -1, -1, -1, -1};
        apu.Reset();

        const size_t maxFlatCopy =
            (img.loadAddr < 0x10000U) ? std::min<size_t>(img.payload.size(), 0x10000U - img.loadAddr) : 0U;
        std::copy_n(img.payload.data(), maxFlatCopy, flatMemory.data() + img.loadAddr);

        if (img.loadAddr >= 0x6000U && img.loadAddr < 0x8000U) {
            const size_t wramOffset = img.loadAddr - 0x6000U;
            const size_t maxWramCopy = std::min<size_t>(img.payload.size(), 0x2000U - wramOffset);
            std::copy_n(img.payload.data(), maxWramCopy, wram.data() + wramOffset);
        }

        const size_t padding = static_cast<size_t>(img.loadAddr & 0x0FFFU);
        const size_t totalPadded = padding + img.payload.size();
        const size_t numBanks = std::max<size_t>(1U, (totalPadded + 0x0FFFU) / 0x1000U);
        banks.assign(numBanks, std::array<uint8_t, 0x1000>{});
        for (size_t i = 0; i < img.payload.size(); ++i) {
            const size_t pos = padding + i;
            banks[pos >> 12U][pos & 0x0FFFU] = img.payload[i];
        }

        if (img.hasExplicitBanks) {
            for (size_t slot = 0; slot < 8U; ++slot) {
                romBanks[slot] = static_cast<int32_t>(img.bankswitchInit[slot]);
            }
        }
    }

    Byte Read(Word address) override {
        if (address < 0x2000U) {
            return nesRam[address & 0x07FFU];
        }
        if (address == 0x4015U) {
            return apu.ReadStatus();
        }
        if (address == kSentinelAddr) {
            return 0x4CU;
        }
        if (address == kSentinelAddr + 1U) {
            return static_cast<uint8_t>(kSentinelAddr & 0xFFU);
        }
        if (address == kSentinelAddr + 2U) {
            return static_cast<uint8_t>((kSentinelAddr >> 8U) & 0xFFU);
        }
        if (address >= 0x5FF6U && address <= 0x5FF7U) {
            const int32_t b = wramBanks[address - 0x5FF6U];
            return static_cast<Byte>(b >= 0 ? b : 0);
        }
        if (address >= 0x5FF8U && address <= 0x5FFFU) {
            const int32_t b = romBanks[address - 0x5FF8U];
            return static_cast<Byte>(b >= 0 ? b : 0);
        }
        if (address >= 0x6000U && address < 0x8000U) {
            const size_t slot = (address - 0x6000U) >> 12U;
            const int32_t bankIdx = wramBanks[slot];
            if (bankIdx >= 0 && static_cast<size_t>(bankIdx) < banks.size()) {
                return banks[static_cast<size_t>(bankIdx)][address & 0x0FFFU];
            }
            return wram[address - 0x6000U];
        }
        if (address >= 0x8000U) {
            const size_t slot = (address - 0x8000U) >> 12U;
            const int32_t bankIdx = romBanks[slot];
            if (bankIdx >= 0 && static_cast<size_t>(bankIdx) < banks.size()) {
                return banks[static_cast<size_t>(bankIdx)][address & 0x0FFFU];
            }
            return flatMemory[address];
        }
        return flatMemory[address];
    }

    void Write(Word address, Byte data) override {
        if (address < 0x2000U) {
            nesRam[address & 0x07FFU] = data;
            return;
        }
        if (address >= 0x4000U && address <= 0x4017U) {
            apu.WriteRegister(address, data);
            return;
        }
        if (address >= 0x5FF6U && address <= 0x5FF7U) {
            wramBanks[address - 0x5FF6U] = static_cast<int32_t>(data);
            return;
        }
        if (address >= 0x5FF8U && address <= 0x5FFFU) {
            romBanks[address - 0x5FF8U] = static_cast<int32_t>(data);
            return;
        }
        if (address >= 0x6000U && address < 0x8000U) {
            const size_t slot = (address - 0x6000U) >> 12U;
            const int32_t bankIdx = wramBanks[slot];
            if (bankIdx >= 0 && static_cast<size_t>(bankIdx) < banks.size()) {
                banks[static_cast<size_t>(bankIdx)][address & 0x0FFFU] = data;
            }
            wram[address - 0x6000U] = data;
        }
    }

    [[nodiscard]] std::string GetName() const override { return "NSFBusDevice"; }
    bool SaveState(std::ostream& out) const override { return out.good(); }
    bool LoadState(std::istream& inStream) override { return inStream.good(); }
};

void CallSubroutine(CPU& cpu, Bus& bus, uint16_t targetAddr, uint32_t maxCycles) {
    cpu.SP = 0x01FFU;
    cpu.PushWord<false>(static_cast<Word>(kSentinelAddr - 1U), bus);
    cpu.PC = targetAddr;
    cpu.isInit = true;
    cpu.waiting = false;
    cpu.UpdatePagePtr(bus);

    uint32_t elapsed = 0U;
    while (cpu.PC != kSentinelAddr && elapsed < maxCycles && !cpu.waiting) {
        cpu.remainingCycles = 0;
        const int status = cpu.StepFast<false>(bus);
        const int stepCycles = std::max(1, cpu.remainingCycles + 1);
        cpu.remainingCycles = 0;
        elapsed += static_cast<uint32_t>(stepCycles);
        if (status != 0) {
            break;
        }
    }
}

bool IsFrameActive(const std::array<NSFChannelFrameState, 4>& ch) {
    return ch[0].volume > 0U || ch[1].volume > 0U || ch[2].volume > 0U || ch[3].volume > 0U;
}

bool ChannelStatesDifferIgnoringRetrigger(const std::array<NSFChannelFrameState, 4>& a,
                                          const std::array<NSFChannelFrameState, 4>& b) {
    for (size_t i = 0; i < 4U; ++i) {
        if (a[i].volume != b[i].volume || a[i].period != b[i].period || a[i].duty != b[i].duty) {
            return true;
        }
    }
    return false;
}

bool HasAnyRetrigger(const std::array<NSFChannelFrameState, 4>& ch) {
    return ch[0].retrigger || ch[1].retrigger || ch[2].retrigger || ch[3].retrigger;
}

uint64_t ComputePitchDutyKey(const std::array<NSFChannelFrameState, 4>& ch) {
    uint64_t key = 0ULL;
    for (size_t i = 0; i < 4U; ++i) {
        uint64_t part = 0ULL;
        if (ch[i].volume > 0U) {
            part = 0x8000ULL | (static_cast<uint64_t>(ch[i].duty & 0x03U) << 11U) |
                   static_cast<uint64_t>(ch[i].period & 0x07FFU);
        }
        key |= (part << (i * 16U));
    }
    return key;
}

bool FramesMatchForLoop(const std::array<NSFChannelFrameState, 4>& a,
                        const std::array<NSFChannelFrameState, 4>& b) {
    for (size_t i = 0; i < 4U; ++i) {
        const bool aActive = a[i].volume > 0U;
        const bool bActive = b[i].volume > 0U;
        if (aActive != bActive) {
            return false;
        }
        if (!aActive) {
            continue;
        }
        if (a[i].duty != b[i].duty) {
            return false;
        }
        const int periodDiff = std::abs(static_cast<int>(a[i].period) - static_cast<int>(b[i].period));
        if ((i < 3U && periodDiff > 1) || (i == 3U && periodDiff > 0)) {
            return false;
        }
        if (std::abs(static_cast<int>(a[i].volume) - static_cast<int>(b[i].volume)) > 3) {
            return false;
        }
    }
    return true;
}

}  // namespace

NSFRunResult NSFRunner::RunFile(const std::string& nsfPath, const NSFRunConfig& config) {
    std::ifstream in(nsfPath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        NSFRunResult res;
        res.errorMessage = "Failed to open NSF file: " + nsfPath;
        return res;
    }
    const auto fileSize = in.tellg();
    if (fileSize <= 0) {
        NSFRunResult res;
        res.errorMessage = "NSF file is empty: " + nsfPath;
        return res;
    }
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return RunFromMemory(buffer, config);
}

NSFRunResult NSFRunner::RunFromMemory(std::span<const uint8_t> nsfData, const NSFRunConfig& config) {
    NSFRunResult result;
    const ParsedNSFImage parsed = ParseNSF(nsfData, config.songIndex);
    if (!parsed.valid) {
        result.errorMessage = parsed.error;
        return result;
    }

    const bool isPal = (parsed.palNtscBits & 0x03U) == 0x01U;
    const uint16_t speedUs = isPal ? ((parsed.playSpeedPal > 0U) ? parsed.playSpeedPal : 19997U)
                                   : ((parsed.playSpeedNtsc > 0U) ? parsed.playSpeedNtsc : 16639U);
    result.fps = 1000000.0 / static_cast<double>(speedUs);
    result.songName = parsed.songName;
    result.artist = parsed.artist;
    result.copyright = parsed.copyright;

    int songOneBased = (config.songIndex > 0) ? config.songIndex : static_cast<int>(parsed.startingSong);
    songOneBased = std::clamp(songOneBased, 1, static_cast<int>(parsed.totalSongs));
    const auto songZeroBased = static_cast<uint8_t>(songOneBased - 1);

    auto bus = std::make_unique<Bus>();
    auto nsfDevice = std::make_unique<NSFBusDevice>();
    nsfDevice->LoadImage(parsed);
    bus->RegisterDevice(0x0000U, 0xFFFFU, nsfDevice.get());

    CPU cpu;
    cpu.Reset();
    cpu.SetCycleAccurate(false);

    for (uint16_t reg = 0x4000U; reg <= 0x4013U; ++reg) {
        nsfDevice->Write(reg, 0x00U);
    }
    nsfDevice->Write(0x4015U, 0x0FU);

    cpu.A = songZeroBased;
    cpu.X = isPal ? 1U : 0U;
    cpu.Y = 0U;
    CallSubroutine(cpu, *bus, parsed.initAddr, 2000000U);

    double capSec = 300.0;
    std::string defaultStopReason = "max_duration";
    if (config.maxDurationSec > 0.0) {
        capSec = config.maxDurationSec;
    } else if (parsed.metaDurationSec > 0.0) {
        capSec = parsed.metaDurationSec;
        defaultStopReason = "metadata";
    }

    const auto maxFrames = std::max<uint32_t>(1U, static_cast<uint32_t>(std::ceil(capSec * result.fps)));
    const auto silenceLimitFrames = static_cast<uint32_t>(std::ceil(2.5 * result.fps));
    const auto haltLimitFrames = static_cast<uint32_t>(std::ceil(3.0 * result.fps));
    const auto minLoopFrames = static_cast<uint32_t>(std::ceil(8.0 * result.fps));
    const auto twentySecFrames = static_cast<uint32_t>(std::ceil(20.0 * result.fps));

    std::vector<std::array<NSFChannelFrameState, 4>> denseStates;
    denseStates.reserve(std::min<uint32_t>(maxFrames, 18000U));
    std::vector<uint64_t> pitchKeys;
    pitchKeys.reserve(std::min<uint32_t>(maxFrames, 18000U));
    std::unordered_map<uint64_t, std::vector<uint32_t>> windowIndexMap;

    int32_t firstActiveFrame = -1;
    uint32_t lastActiveFrame = 0U;
    uint32_t lastApuWriteFrame = 0U;
    uint32_t effectiveTotalFrames = maxFrames;
    result.stopReason = defaultStopReason;

    std::array<NSFChannelFrameState, 4> prevChannels{};

    for (uint32_t frame = 0U; frame < maxFrames; ++frame) {
        nsfDevice->apu.BeginFrame();
        CallSubroutine(cpu, *bus, parsed.playAddr, 200000U);

        const auto currentChannels = nsfDevice->apu.EvaluateFrameState(prevChannels, frame == 0U);
        nsfDevice->apu.StepFrameSequencer();

        denseStates.push_back(currentChannels);
        pitchKeys.push_back(ComputePitchDutyKey(currentChannels));

        if (frame == 0U || ChannelStatesDifferIgnoringRetrigger(currentChannels, prevChannels) ||
            HasAnyRetrigger(currentChannels)) {
            result.frames.push_back(NSFFrameRecord{
                .frameIndex = frame,
                .channels = currentChannels,
            });
        }
        prevChannels = currentChannels;

        const bool active = IsFrameActive(currentChannels);
        if (active) {
            if (firstActiveFrame < 0) {
                firstActiveFrame = static_cast<int32_t>(frame);
            }
            lastActiveFrame = frame;
        }
        if (nsfDevice->apu.wroteAnyRegisterThisFrame) {
            lastApuWriteFrame = frame;
        }

        if (firstActiveFrame >= 0) {
            if (frame > lastActiveFrame && (frame - lastActiveFrame) >= silenceLimitFrames) {
                result.stopReason = "silence";
                effectiveTotalFrames = std::min<uint32_t>(frame + 1U, lastActiveFrame + 6U);
                break;
            }
            if (frame > lastApuWriteFrame && (frame - lastApuWriteFrame) >= haltLimitFrames) {
                result.stopReason = "halt";
                effectiveTotalFrames =
                    std::min<uint32_t>(frame + 1U, std::max(lastActiveFrame, lastApuWriteFrame) + 6U);
                break;
            }
        }

        constexpr uint32_t kHashWindow = 16U;
        if (firstActiveFrame >= 0 && frame >= static_cast<uint32_t>(firstActiveFrame) + kHashWindow) {
            uint64_t hash = 14695981039346656037ULL;
            uint32_t nonSilentCount = 0U;
            for (uint32_t k = frame + 1U - kHashWindow; k <= frame; ++k) {
                hash ^= pitchKeys[k];
                hash *= 1099511628211ULL;
                if (pitchKeys[k] != 0ULL) {
                    ++nonSilentCount;
                }
            }

            if (nonSilentCount >= (kHashWindow / 2U)) {
                auto& bucket = windowIndexMap[hash];
                bool foundLoop = false;
                for (uint32_t prevFrame : bucket) {
                    const uint32_t period = frame - prevFrame;
                    if (period < minLoopFrames) {
                        continue;
                    }
                    // Short periods (< 20s) require period + 2s lookahead so 2x tandem repeats (A, A, B) never false-trigger
                    const uint32_t lookahead =
                        (period < twentySecFrames)
                            ? (period + static_cast<uint32_t>(std::ceil(2.0 * result.fps)))
                            : std::clamp<uint32_t>(period / 2U, static_cast<uint32_t>(std::ceil(15.0 * result.fps)),
                                                   static_cast<uint32_t>(std::ceil(25.0 * result.fps)));

                    if (frame < static_cast<uint32_t>(firstActiveFrame) + period + lookahead) {
                        continue;
                    }

                    const uint32_t checkStart = frame - lookahead + 1U;
                    uint32_t matchedFrames = 0U;
                    for (uint32_t idx = checkStart; idx <= frame; ++idx) {
                        if (FramesMatchForLoop(denseStates[idx], denseStates[idx - period])) {
                            ++matchedFrames;
                        }
                    }
                    if (matchedFrames * 100U < lookahead * 92U) {
                        continue;
                    }

                    uint32_t loopEnd = checkStart;
                    const uint32_t minLoopEnd = static_cast<uint32_t>(firstActiveFrame) + period;
                    while (loopEnd > minLoopEnd) {
                        if (FramesMatchForLoop(denseStates[loopEnd - 1U], denseStates[loopEnd - 1U - period])) {
                            --loopEnd;
                        } else {
                            break;
                        }
                    }
                    const uint32_t loopStart = loopEnd - period;

                    uint32_t activeInLoop = 0U;
                    for (uint32_t idx = loopStart; idx < loopEnd; ++idx) {
                        if (pitchKeys[idx] != 0ULL) {
                            ++activeInLoop;
                        }
                    }
                    if (activeInLoop * 4U < period) {
                        continue;
                    }

                    result.stopReason = "loop";
                    result.loopStartFrame = static_cast<int32_t>(loopStart);
                    result.loopEndFrame = static_cast<int32_t>(loopEnd);
                    effectiveTotalFrames = loopEnd;
                    foundLoop = true;
                    break;
                }
                if (foundLoop) {
                    break;
                }
                if (bucket.size() < 24U) {
                    bucket.push_back(frame);
                }
            }
        }
    }

    std::erase_if(result.frames,
                  [effectiveTotalFrames](const NSFFrameRecord& rec) { return rec.frameIndex >= effectiveTotalFrames; });

    result.totalFrames = effectiveTotalFrames;
    result.durationSec = static_cast<double>(effectiveTotalFrames) / result.fps;
    result.success = true;
    return result;
}

bool NSFRunner::WriteTraceJson(const NSFRunResult& result, const std::string& outputPath) {
    if (!result.success) {
        return false;
    }
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    out << std::fixed << std::setprecision(6);
    out << "{\n"
        << "  \"fps\": " << result.fps << ",\n"
        << "  \"total_frames\": " << result.totalFrames << ",\n"
        << "  \"duration_sec\": " << result.durationSec << ",\n"
        << "  \"stop_reason\": " << EscapeJsonString(result.stopReason) << ",\n"
        << "  \"loop_start_frame\": " << result.loopStartFrame << ",\n"
        << "  \"loop_end_frame\": " << result.loopEndFrame << ",\n"
        << "  \"song_name\": " << EscapeJsonString(result.songName) << ",\n"
        << "  \"artist\": " << EscapeJsonString(result.artist) << ",\n"
        << "  \"copyright\": " << EscapeJsonString(result.copyright) << ",\n"
        << "  \"frames\": [\n";

    for (size_t i = 0; i < result.frames.size(); ++i) {
        const auto& rec = result.frames[i];
        out << "    [" << rec.frameIndex;
        for (size_t ch = 0; ch < 4U; ++ch) {
            const auto& c = rec.channels[ch];
            out << ", [" << static_cast<uint32_t>(c.volume) << ", " << c.period << ", "
                << static_cast<uint32_t>(c.duty) << ", " << (c.retrigger ? 1 : 0) << "]";
        }
        out << "]" << ((i + 1U < result.frames.size()) ? ",\n" : "\n");
    }

    out << "  ]\n}\n";
    return out.good();
}

bool NSFRunner::RunAndWriteTrace(const std::string& nsfPath, const std::string& outputPath,
                                 const NSFRunConfig& config) {
    const NSFRunResult res = RunFile(nsfPath, config);
    if (!res.success) {
        std::cerr << "[NSFRunner] Error: " << res.errorMessage << '\n';
        return false;
    }
    if (!WriteTraceJson(res, outputPath)) {
        std::cerr << "[NSFRunner] Failed to write trace JSON to: " << outputPath << '\n';
        return false;
    }
    return true;
}

}  // namespace Hardware
