#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class SDCard : public IBusDevice {
   public:
    SDCard();
    void Reset() override;
    ~SDCard() override;

    SDCard(const SDCard&) = delete;
    SDCard& operator=(const SDCard&) = delete;
    SDCard(SDCard&&) = delete;
    SDCard& operator=(SDCard&&) = delete;

    // Sets the path and opens the ISO. Returns false if fails.
    bool Mount(const std::string& imagePath);
    void Unmount();
    [[nodiscard]] bool IsMounted() const { return mounted; }
    [[nodiscard]] std::string GetMountedPath() const { return currentPath; }

    // IBusDevice overrides
    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    [[nodiscard]] std::string GetName() const override { return "SD Card"; }
    [[nodiscard]] bool IsReadOnly() const override { return is_read_only; }
    Byte* GetRawMemory() override { return nullptr; }

    // SPI interface
    // CS must be low to communicate
    void SetCS(bool csVal);

    // Transfers one byte (receives MOSI, returns MISO)
    uint8_t TransferByte(uint8_t mosi);

    void SetReadLatencyEnabled(bool enabled) { read_latency_enabled = enabled; }
    [[nodiscard]] bool IsReadLatencyEnabled() const { return read_latency_enabled; }

    // State management
    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inStream) override;

   private:
    std::fstream imageFile;
    bool mounted = false;
    std::string currentPath;

    bool is_read_only = false;
    bool cs_active = false;  // Active low

    // SPI State Machine
    enum class State : std::uint8_t {
        IDLE,
        COMMAND_RECEIVE,
        COMMAND_EXECUTE,
        WAIT_RESPONSE,
        SEND_RESPONSE,
        READ_DATA_DELAY,
        READ_DATA_TOKEN,
        READ_DATA_BLOCK,
        READ_DATA_CRC,
        WRITE_DATA_TOKEN,
        WRITE_DATA_BLOCK,
        WRITE_DATA_CRC,
        WRITE_BUSY
    };

    State state = State::IDLE;
    uint8_t last_miso = 0xFFU;

    // Command parsing
    std::array<std::uint8_t, 6> cmd_buffer{};
    int cmd_bytes_received = 0;

    // Response sending
    std::vector<uint8_t> response_buffer;
    int response_index = 0;

    // Data reading/writing
    std::array<std::uint8_t, 512> data_buffer{};
    int data_index = 0;
    uint32_t current_lba = 0;  // Current Logical Block Address

    // Internal flags
    bool is_acmd = false;
    bool is_initialized = false;
    bool is_sdhc = false;

    uint8_t warmup_bytes = 0;
    uint8_t acmd41_attempts = 0;
    uint8_t acmd41_target_attempts = 0;
    uint8_t write_busy_bytes = 0;
    uint8_t read_delay_bytes = 0;
    
    bool crc_enabled = false;
    bool read_latency_enabled = false;
    uint16_t current_crc = 0;
    uint16_t received_crc = 0;

    // Helpers
    uint8_t CalculateCrc7(const std::array<std::uint8_t, 6>& buffer) const;
    uint16_t CalculateCrc16(const std::array<std::uint8_t, 512>& buffer) const;
    void ProcessCommand();
    void HandleAcmd(uint8_t cmd, uint32_t arg);
    void HandleStandardCmd(uint8_t cmd, uint32_t arg);
    void HandleAcmd41();
    void HandleCmd0();
    void HandleCmd8(uint32_t arg);
    void HandleCmd16();
    void HandleCmd17(uint32_t arg);
    void HandleCmd24(uint32_t arg);
    void HandleCmd55();
    void HandleCmd58();
    void HandleCmd59(uint32_t arg);

    void QueueResponse1(uint8_t response1);
    void QueueResponse2(uint8_t response1, uint8_t response2);
    void QueueResponse3(uint8_t response1, uint32_t response3);
    void QueueResponse7(uint8_t response1, uint32_t response7);
    void ReadBlockFromImage();
    void WriteBlockToImage();

    // SPI State Handlers
    void HandleIdleState(uint8_t mosi);
    void HandleCommandReceiveState(uint8_t mosi);
    void HandleWaitResponseState(uint8_t& miso);
    void HandleSendResponseState(uint8_t& miso);
    void HandleReadDataDelayState(uint8_t& miso);
    void HandleReadDataTokenState(uint8_t& miso);
    void HandleReadDataBlockState(uint8_t& miso);
    void HandleReadDataCrcState(uint8_t& miso);
    void HandleWriteDataTokenState(uint8_t mosi);
    void HandleWriteDataBlockState(uint8_t mosi);
    void HandleWriteDataCrcState(uint8_t& miso);
    void HandleWriteBusyState(uint8_t& miso);
};

}  // namespace Hardware
