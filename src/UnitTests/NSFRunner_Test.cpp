#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "Hardware/Audio/NSFRunner.h"

namespace {

std::vector<uint8_t> BuildSyntheticNSFHeader(uint16_t loadAddr, uint16_t initAddr, uint16_t playAddr,
                                             const std::array<uint8_t, 8>& banks = {}) {
    std::vector<uint8_t> header(128U, 0U);
    header[0] = 'N';
    header[1] = 'E';
    header[2] = 'S';
    header[3] = 'M';
    header[4] = 0x1AU;
    header[5] = 1U;
    header[6] = 1U;
    header[7] = 1U;
    header[8] = static_cast<uint8_t>(loadAddr & 0xFFU);
    header[9] = static_cast<uint8_t>((loadAddr >> 8U) & 0xFFU);
    header[10] = static_cast<uint8_t>(initAddr & 0xFFU);
    header[11] = static_cast<uint8_t>((initAddr >> 8U) & 0xFFU);
    header[12] = static_cast<uint8_t>(playAddr & 0xFFU);
    header[13] = static_cast<uint8_t>((playAddr >> 8U) & 0xFFU);

    const char* title = "UnitTestSong";
    std::memcpy(header.data() + 0x0EU, title, std::strlen(title));
    header[0x6EU] = 0xFFU;
    header[0x6FU] = 0x40U;
    for (size_t i = 0; i < 8U; ++i) {
        header[0x70U + i] = banks[i];
    }
    return header;
}

}  // namespace

TEST(NSFRunnerTest, BankswitchingAndAPUState) {
    // 2 x 4KB banks: initially slot 0 ($8000) = bank 0, slot 1 ($9000) = bank 1.
    // INIT at $9000 switches bank 1 into slot 0 ($8000) via STA $5FF8.
    auto nsf = BuildSyntheticNSFHeader(0x8000U, 0x9000U, 0x8000U, {0U, 1U, 0U, 0U, 0U, 0U, 0U, 0U});
    std::vector<uint8_t> payload(0x2000U, 0x60U);

    const std::vector<uint8_t> bank1Code = {
        0xA5, 0x10,        // LDA $10
        0xD0, 0x08,        // BNE play_entry (+8 bytes)
        0xE6, 0x10,        // INC $10 (2)
        0xA9, 0x01,        // LDA #$01 (2)
        0x8D, 0xF8, 0x5F,  // STA $5FF8 (3)
        0x60,              // RTS (1)
        // play_entry:
        0xA5, 0x11,        // LDA $11
        0xD0, 0x2F,        // BNE play_done (+47 bytes)
        0xE6, 0x11,        // INC $11 (2)
        0xA9, 0xBF,        // LDA #$BF (duty 2, halt=1, constVol=1, vol=15)
        0x8D, 0x00, 0x40,  // STA $4000
        0xA9, 0xFD,        // LDA #$FD
        0x8D, 0x02, 0x40,  // STA $4002
        0xA9, 0x08,        // LDA #$08
        0x8D, 0x03, 0x40,  // STA $4003
        0xA9, 0xFF,        // LDA #$FF (control=1, linear=127)
        0x8D, 0x08, 0x40,  // STA $4008
        0xA9, 0xFC,        // LDA #$FC
        0x8D, 0x0A, 0x40,  // STA $400A
        0xA9, 0x09,        // LDA #$09 (period high = 1 -> 0x1FC = 508)
        0x8D, 0x0B, 0x40,  // STA $400B
        0xA9, 0x38,        // LDA #$38 (halt=1, constVol=1, vol=8)
        0x8D, 0x0C, 0x40,  // STA $400C
        0xA9, 0x83,        // LDA #$83 (mode=1, period=3)
        0x8D, 0x0E, 0x40,  // STA $400E
        0xA9, 0x08,        // LDA #$08
        0x8D, 0x0F, 0x40,  // STA $400F
        // play_done:
        0x60               // RTS
    };
    std::copy(bank1Code.begin(), bank1Code.end(), payload.begin() + 0x1000U);
    nsf.insert(nsf.end(), payload.begin(), payload.end());

    const auto res = Hardware::NSFRunner::RunFromMemory(nsf, {.maxDurationSec = 0.1, .songIndex = 1});
    ASSERT_TRUE(res.success);
    ASSERT_FALSE(res.frames.empty());

    const auto& f0 = res.frames.front();
    EXPECT_EQ(f0.frameIndex, 0U);
    EXPECT_EQ(f0.channels[0].volume, 15U);
    EXPECT_EQ(f0.channels[0].period, 253U);
    EXPECT_EQ(f0.channels[0].duty, 2U);
    EXPECT_TRUE(f0.channels[0].retrigger);

    EXPECT_EQ(f0.channels[1].volume, 0U);

    EXPECT_EQ(f0.channels[2].volume, 15U);
    EXPECT_EQ(f0.channels[2].period, 508U);
    EXPECT_EQ(f0.channels[2].duty, 0U);
    EXPECT_TRUE(f0.channels[2].retrigger);

    EXPECT_EQ(f0.channels[3].volume, 8U);
    EXPECT_EQ(f0.channels[3].period, 3U);
    EXPECT_EQ(f0.channels[3].duty, 1U);
    EXPECT_TRUE(f0.channels[3].retrigger);
}

TEST(NSFRunnerTest, HardwareEnvelopeAndLengthCounter) {
    auto nsf = BuildSyntheticNSFHeader(0x8000U, 0x8000U, 0x8005U);
    const std::vector<uint8_t> code = {
        // INIT ($8000):
        0xA9, 0x00, 0x85, 0x00, 0x60,
        // PLAY ($8005):
        0xA5, 0x00,        // LDA $00
        0xD0, 0x20,        // BNE done (+32 bytes)
        0xE6, 0x00,        // INC $00 (2)
        // Pulse 1: hardware envelope (constVol=0, period=0, lengthHalt=0), timer=100, lengthIdx=0 (10 -> 5 frames)
        0xA9, 0x00,        // LDA #$00
        0x8D, 0x00, 0x40,  // STA $4000
        0xA9, 0x64,        // LDA #100
        0x8D, 0x02, 0x40,  // STA $4002
        0xA9, 0x00,        // LDA #$00 (length table index 0 -> 10)
        0x8D, 0x03, 0x40,  // STA $4003
        // Pulse 2: constant volume 12, lengthHalt=0, timer=200, lengthIdx=3 (2 -> 1 frame)
        0xA9, 0x1C,        // LDA #$1C
        0x8D, 0x04, 0x40,  // STA $4004
        0xA9, 0xC8,        // LDA #200
        0x8D, 0x06, 0x40,  // STA $4006
        0xA9, 0x18,        // LDA #$18 (length table index 3 -> 2)
        0x8D, 0x07, 0x40,  // STA $4007
        // done:
        0x60               // RTS
    };

    nsf.insert(nsf.end(), code.begin(), code.end());

    const auto res = Hardware::NSFRunner::RunFromMemory(nsf, {.maxDurationSec = 0.2, .songIndex = 1});
    ASSERT_TRUE(res.success);
    ASSERT_GE(res.frames.size(), 5U);

    EXPECT_EQ(res.frames[0].frameIndex, 0U);
    EXPECT_EQ(res.frames[0].channels[0].volume, 15U);
    EXPECT_EQ(res.frames[0].channels[1].volume, 12U);

    EXPECT_EQ(res.frames[1].frameIndex, 1U);
    EXPECT_EQ(res.frames[1].channels[0].volume, 11U);
    EXPECT_EQ(res.frames[1].channels[1].volume, 0U);

    EXPECT_EQ(res.frames[2].channels[0].volume, 7U);
    EXPECT_EQ(res.frames[3].channels[0].volume, 3U);
    EXPECT_EQ(res.frames[4].channels[0].volume, 0U);
}

TEST(NSFRunnerTest, SilenceDetectionAndTrim) {
    auto nsf = BuildSyntheticNSFHeader(0x8000U, 0x8000U, 0x8005U);
    const std::vector<uint8_t> rom = {
        // INIT ($8000):
        0xA9, 0x00,        // LDA #$00
        0x85, 0x00,        // STA $00
        0x60,              // RTS
        // PLAY ($8005):
        0xA5, 0x00,        // LDA $00
        0xC9, 0x00,        // CMP #$00
        0xD0, 0x0F,        // BNE check_mute (+15 bytes)
        // Frame 0: start constant note on Pulse 1
        0xA9, 0x3F,        // LDA #$3F (halt=1, constVol=1, vol=15)
        0x8D, 0x00, 0x40,  // STA $4000
        0xA9, 0x80,        // LDA #$80
        0x8D, 0x02, 0x40,  // STA $4002
        0xA9, 0x08,        // LDA #$08
        0x8D, 0x03, 0x40,  // STA $4003
        // check_mute:
        0xA5, 0x00,        // LDA $00
        0xC9, 0x1E,        // CMP #30
        0xD0, 0x05,        // BNE inc_and_ret (+5 bytes)
        0xA9, 0x00,        // LDA #$00
        0x8D, 0x15, 0x40,  // STA $4015 (silence all channels at frame 30)
        // inc_and_ret:
        0xE6, 0x00,        // INC $00
        0xD0, 0x02,        // BNE done (+2 bytes)
        0xC6, 0x00,        // DEC $00 (clamp at 255)
        // done:
        0x60               // RTS
    };
    nsf.insert(nsf.end(), rom.begin(), rom.end());

    const auto res = Hardware::NSFRunner::RunFromMemory(nsf);
    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.stopReason, "silence");
    EXPECT_EQ(res.totalFrames, 35U);
}

TEST(NSFRunnerTest, TrueLoopDetectionRejectsInternalRepeat) {
    // Plays sequence A (512 frames), A (512 frames), B (512 frames) = 1536 frames total cycle, repeating forever.
    auto nsf = BuildSyntheticNSFHeader(0x8000U, 0x8000U, 0x8007U);
    std::vector<uint8_t> image(0x60U, 0x60U);
    const std::vector<uint8_t> exactCode = {
        // $8000 INIT:
        0x64, 0x00,                            // STZ $00
        0x64, 0x01,                            // STZ $01
        0x60,                                  // RTS
        0xEA, 0xEA,                            // NOP, NOP (pad to $8007)
        // $8007 PLAY:
        0xA5, 0x00,                            // LDA $00
        0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A,    // LSR A x6 -> 0..3
        0x85, 0x02,                            // STA $02
        0xA5, 0x01,                            // LDA $01
        0xC9, 0x04,                            // CMP #$04
        0xB0, 0x09,                            // BCS sec_b (+9 bytes)
        0x29, 0x01,                            // AND #$01 (2)
        0x0A, 0x0A,                            // ASL A x2 (2)
        0x18,                                  // CLC (1)
        0x65, 0x02,                            // ADC $02 (2)
        0x80, 0x0A,                            // BRA write_apu (+10 bytes) (2)
        // sec_b:
        0x29, 0x01,                            // AND #$01 (2)
        0x0A, 0x0A,                            // ASL A x2 (2)
        0x18,                                  // CLC (1)
        0x65, 0x02,                            // ADC $02 (2)
        0x18,                                  // CLC (1)
        0x69, 0x08,                            // ADC #$08 (2)
        // write_apu:
        0xAA,                                  // TAX
        0xBD, 0x40, 0x80,                      // LDA $8040,X
        0x8D, 0x02, 0x40,                      // STA $4002
        0xA9, 0x3C,                            // LDA #$3C
        0x8D, 0x00, 0x40,                      // STA $4000
        0xA9, 0x08,                            // LDA #$08
        0x8D, 0x03, 0x40,                      // STA $4003
        0xE6, 0x00,                            // INC $00
        0xD0, 0x0A,                            // BNE done (+10 bytes)
        0xE6, 0x01,                            // INC $01 (2)
        0xA5, 0x01,                            // LDA $01 (2)
        0xC9, 0x06,                            // CMP #$06 (2)
        0x90, 0x02,                            // BCC done (+2 bytes) (2)
        0x64, 0x01,                            // STZ $01 (2)
        // done:
        0x60                                   // RTS
    };
    ASSERT_LE(exactCode.size(), 0x40U);
    std::copy(exactCode.begin(), exactCode.end(), image.begin());

    const std::array<uint8_t, 16> noteTable = {
        40U,  50U,  60U,  70U,  80U,  90U,  100U, 110U,
        150U, 165U, 180U, 195U, 210U, 225U, 240U, 250U,
    };
    std::copy(noteTable.begin(), noteTable.end(), image.begin() + 0x40U);

    nsf.insert(nsf.end(), image.begin(), image.end());

    const auto res = Hardware::NSFRunner::RunFromMemory(nsf);
    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.stopReason, "loop");
    EXPECT_EQ(res.loopStartFrame, 0);
    EXPECT_EQ(res.loopEndFrame, 1536);
    EXPECT_EQ(res.totalFrames, 1536U);
}
