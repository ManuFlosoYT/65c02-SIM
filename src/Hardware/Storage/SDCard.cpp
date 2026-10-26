#include "Hardware/Storage/SDCard.h"
#include "Hardware/Storage/VirtualFAT16Synchronizer.h"
#include "Hardware/Core/ISerializable.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string_view>

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown"
#endif

namespace Hardware {

namespace {

// Paths are stored as UTF-8; on Windows std::string -> path would use the ANSI code page.
std::filesystem::path PathFromUtf8(std::string_view str) {
    return std::filesystem::path(std::u8string(str.begin(), str.end()));
}

}  // namespace

SDCard::SDCard() { Reset(); }

SDCard::~SDCard() { Unmount(); }

void SDCard::Reset() {
    state = State::IDLE;
    cmd_bytes_received = 0;
    response_index = 0;
    response_buffer.clear();
    is_acmd = false;
    is_initialized = false;
    is_sdhc = true;  // Assuming SDHC for 512-byte sectors and blocks addressing
    cs_active = false;
    crc_enabled = false;
    is_read_multiblock = false;
    is_write_multiblock = false;
    sd_status_byte = 0x00;
    last_miso = 0xFFU;
    last_mosi = 0xFFU;

    if (is_folder_mount && !currentPath.empty()) {
        SyncFromHost();
    }
}

bool SDCard::Mount(const std::string& imagePath) {
    const std::filesystem::path fsPath = PathFromUtf8(imagePath);
    if (std::filesystem::is_directory(fsPath)) {
        return MountFolder(imagePath);
    }

    if (mounted) {
        Unmount();
    }

    imageFile.open(fsPath, std::ios::in | std::ios::out | std::ios::binary);
    if (imageFile.is_open()) {
        is_read_only = false;
    } else {
        // Try opening read-only if read-write failed
        imageFile.clear();
        imageFile.open(fsPath, std::ios::in | std::ios::binary);
        if (imageFile.is_open()) {
            is_read_only = true;
        } else {
            // Try creating it if it doesn't exist
            imageFile.clear();
            imageFile.open(fsPath, std::ios::out | std::ios::binary);
            if (imageFile.is_open()) {
                imageFile.close();
                imageFile.open(fsPath, std::ios::in | std::ios::out | std::ios::binary);
                is_read_only = false;
            }
        }
    }

    if (imageFile.is_open()) {
        currentPath = imagePath;
        mounted = true;
        
        imageFile.seekg(0, std::ios::end);
        total_blocks = static_cast<uint32_t>(imageFile.tellg() / 512);
        imageFile.seekg(0, std::ios::beg);
        
        is_modified = false;
        Reset();
        return true;
    }
    return false;
}

void SDCard::Unmount() {
    sfnToHostMap.clear();
    if (is_folder_mount) {
        if (is_modified) {
            SyncToHost();
        }
        virtual_disk.clear();
        virtual_disk.shrink_to_fit();
        is_folder_mount = false;
        allow_host_deletion = false;
    }
    if (imageFile.is_open()) {
        imageFile.close();
    }
    mounted = false;
    is_modified = false;
}

bool SDCard::MountFolder(const std::string& folderPath, bool allowDeletion) {
    const std::filesystem::path hostFolder = PathFromUtf8(folderPath);
    if (!std::filesystem::exists(hostFolder) || !std::filesystem::is_directory(hostFolder)) {
        return false;
    }

    std::error_code ec;
    std::filesystem::path canPath = std::filesystem::canonical(hostFolder, ec);
    if (ec || VirtualFAT16Synchronizer::IsProtectedHostPath(canPath)) {
        return false;
    }

    if (std::filesystem::exists(hostFolder / ".65C02_SIM_mount")) {
        allow_host_deletion = true;
    } else {
        allow_host_deletion = allowDeletion;
    }

    if (mounted) {
        Unmount();
    }

    mounted = true;
    is_folder_mount = true;
    currentPath = folderPath;
    total_blocks = 65536;
    is_read_only = false;
    is_modified = false;

    if (!VirtualFAT16Synchronizer::Build(virtual_disk, hostFolder, sfnToHostMap)) {
        Unmount();
        return false;
    }

    Reset();
    return true;
}

bool SDCard::SyncFromHost() {
    if (!is_folder_mount || currentPath.empty()) {
        return false;
    }
    if (is_modified) {
        SyncToHost();
    }
    return VirtualFAT16Synchronizer::Build(virtual_disk, PathFromUtf8(currentPath), sfnToHostMap);
}

bool SDCard::SyncToHost() {
    if (!is_folder_mount || currentPath.empty()) {
        return false;
    }
    VirtualFAT16Synchronizer::Extract(virtual_disk, PathFromUtf8(currentPath), sfnToHostMap, allow_host_deletion);
    is_modified = false;
    return true;
}

void SDCard::SetCS(bool csVal) {
    // CS is active low
    cs_active = !csVal;

    if (!cs_active) {
        if (state != State::WRITE_BUSY && state != State::CMD12_BUSY) {
            state = State::IDLE;
            cmd_bytes_received = 0;
            response_buffer.clear();
        }
    }
}

Byte SDCard::Read(Word address) {
    switch (address & 0x03U) {
        case 0:  // CTRL
            return (mounted ? 0x01U : 0x00U);
        case 1:  // DATA
            return last_miso;
        default:
            return 0xFFU;
    }
}

void SDCard::Write(Word address, Byte data) {
    switch (address & 0x03U) {
        case 0:  // CTRL
            SetCS((data & 0x01U) == 0); // If 0, then high (inactive), if 1, then low (active)
            break;
        case 1:  // DATA
            last_mosi = data;
            last_miso = TransferByte(data);
            break;
        default:
            break;
    }
}

uint8_t SDCard::TransferByte(uint8_t mosi) {
    if (!mounted) {
        return 0xFFU;
    }
    if (!cs_active) {
        if (!is_initialized && warmup_bytes < 10) {
            warmup_bytes++;
        }
        return 0xFFU;
    }

    uint8_t miso = 0xFFU;
    switch (state) {
        case State::IDLE:
            HandleIdleState(mosi);
            break;
        case State::COMMAND_RECEIVE:
            HandleCommandReceiveState(mosi);
            break;
        case State::WAIT_RESPONSE:
            HandleWaitResponseState(miso);
            break;
        case State::SEND_RESPONSE:
            HandleSendResponseState(miso);
            break;
        case State::READ_PENDING:
            HandleReadPendingState(mosi, miso);
            break;
        case State::READ_DATA_DELAY:
            HandleReadDataDelayState(mosi, miso);
            break;
        case State::READ_DATA_TOKEN:
            HandleReadDataTokenState(mosi, miso);
            break;
        case State::READ_DATA_BLOCK:
            HandleReadDataBlockState(mosi, miso);
            break;
        case State::READ_DATA_CRC:
            HandleReadDataCrcState(mosi, miso);
            break;
        case State::WRITE_DATA_TOKEN:
            HandleWriteDataTokenState(mosi);
            break;
        case State::WRITE_DATA_BLOCK:
            HandleWriteDataBlockState(mosi);
            break;
        case State::WRITE_DATA_CRC:
            HandleWriteDataCrcState(mosi, miso);
            break;
        case State::WRITE_BUSY:
            HandleWriteBusyState(miso);
            break;
        case State::CMD12_BUSY:
            HandleCmd12BusyState(miso);
            break;
        default:
            state = State::IDLE;
            break;
    }
    return miso;
}

void SDCard::HandleIdleState(uint8_t mosi) {
    if ((mosi & 0xC0U) == 0x40U) {  // Command starts with 01
        if (!is_initialized && warmup_bytes < 10) {
            return; // Ignore command if warmup is not complete
        }
        cmd_buffer[0] = mosi;
        cmd_bytes_received = 1;
        state = State::COMMAND_RECEIVE;
    }
}

void SDCard::HandleCommandReceiveState(uint8_t mosi) {
    cmd_buffer.at(static_cast<size_t>(cmd_bytes_received++)) = mosi;
    if (cmd_bytes_received == 6) {
        ProcessCommand();
        // State is now WAIT_RESPONSE (from ProcessCommand's QueueResponse calls)
    }
}

void SDCard::HandleWaitResponseState(uint8_t& miso) {
    // Standard commands require a 1-8 byte delay before sending the response.
    // Returning 0xFF here provides a 1-byte gap, making the response
    // available in the very next byte transfer.
    miso = 0xFFU;
    state = State::SEND_RESPONSE;
}

void SDCard::HandleSendResponseState(uint8_t& miso) {
    if (static_cast<size_t>(response_index) < response_buffer.size()) {
        miso = response_buffer.at(static_cast<size_t>(response_index++));
        if (static_cast<size_t>(response_index) == response_buffer.size()) {
            response_buffer.clear();
            // Transition based on the last command
            uint8_t last_cmd = cmd_buffer[0] & 0x3FU;
            if ((last_cmd == 24U || last_cmd == 25U) && !is_acmd) {
                state = State::WRITE_DATA_TOKEN;
            } else if ((last_cmd == 17U || last_cmd == 18U) && !is_acmd) {
                state = State::READ_PENDING;
            } else if (last_cmd == 12U && !is_acmd) {
                state = State::CMD12_BUSY;
            } else {
                state = State::IDLE;
                sd_status_byte = 0x00; // Clear status on normal completion
            }
        }
    } else {
        state = State::IDLE;
        sd_status_byte = 0x00; // Clear status on normal completion
    }
}

bool SDCard::CheckForStopCommand(uint8_t mosi) {
    if (is_read_multiblock && mosi == 0x4CU) {
        // Abort read
        is_read_multiblock = false;
        cmd_buffer[0] = mosi;
        cmd_bytes_received = 1;
        state = State::COMMAND_RECEIVE;
        return true;
    }
    return false;
}

void SDCard::HandleReadPendingState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    miso = 0xFFU;
    current_crc = CalculateCrc16(data_buffer);
    if (read_latency_enabled) {
        read_delay_bytes = static_cast<uint8_t>((std::rand() % 20) + 5);
    } else {
        read_delay_bytes = 0;
    }
    state = State::READ_DATA_DELAY;
}

void SDCard::HandleReadDataDelayState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    miso = 0xFFU;
    if (read_delay_bytes > 0) {
        read_delay_bytes--;
    } else {
        state = State::READ_DATA_TOKEN;
    }
}

void SDCard::HandleReadDataTokenState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    // Output token 0xFE before data block
    miso = 0xFEU;
    data_index = 0;
    state = State::READ_DATA_BLOCK;
}

void SDCard::HandleReadDataBlockState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    miso = data_buffer.at(static_cast<size_t>(data_index++));
    if (data_index == 512) {
        state = State::READ_DATA_CRC;
        data_index = 0;
    }
}

void SDCard::HandleReadDataCrcState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    if (data_index == 0) {
        miso = static_cast<uint8_t>((current_crc >> 8) & 0xFFU);
    } else {
        miso = static_cast<uint8_t>(current_crc & 0xFFU);
    }
    data_index++;
    if (data_index == 2) {
        if (is_read_multiblock) {
            current_lba++;
            if (current_lba >= total_blocks) {
                state = State::IDLE;
            } else {
                ReadBlockFromImage();
                state = State::READ_PENDING;
            }
        } else {
            state = State::IDLE;
        }
    }
}

void SDCard::HandleWriteDataTokenState(uint8_t mosi) {
    if (mosi == 0xFEU || (is_write_multiblock && mosi == 0xFCU)) {  // Start block token
        data_index = 0;
        state = State::WRITE_DATA_BLOCK;
    } else if (is_write_multiblock && mosi == 0xFDU) { // Stop Tran token
        is_write_multiblock = false;
        write_busy_bytes = 5;
        state = State::WRITE_BUSY;
    } else if ((mosi & 0xF0U) == 0xF0U) {  // Dummy bytes before token
        // Do nothing
    } else {
        // Unexpected token
        state = State::IDLE;
    }
}

void SDCard::HandleWriteDataBlockState(uint8_t mosi) {
    data_buffer.at(static_cast<size_t>(data_index++)) = mosi;
    if (data_index == 512) {
        state = State::WRITE_DATA_CRC;
        data_index = 0;
    }
}

void SDCard::HandleWriteDataCrcState(uint8_t mosi, uint8_t& miso) {
    if (data_index == 0) {
        received_crc = static_cast<uint16_t>(mosi) << 8;
    } else if (data_index == 1) {
        received_crc |= static_cast<uint16_t>(mosi);
    }

    data_index++;
    if (data_index == 3) {
        if (crc_enabled && received_crc != CalculateCrc16(data_buffer)) {
            // CRC Error: xxx01011b = 0x0B -> CRC Error
            miso = 0x0BU;
            state = State::IDLE;
        } else {
            WriteBlockToImage();
            if (is_write_multiblock) {
                current_lba++;
            }
            // Send Data Response:
            // xxx00101b = 0x05 -> Accepted
            miso = 0x05U;
            write_busy_bytes = 5;
            state = State::WRITE_BUSY;
        }
    }
}

void SDCard::HandleWriteBusyState(uint8_t& miso) {
    miso = 0x00U;
    if (write_busy_bytes > 0) {
        write_busy_bytes--;
    } else {
        miso = 0xFFU;
        if (is_write_multiblock) {
            state = State::WRITE_DATA_TOKEN;
        } else {
            state = State::IDLE;
        }
    }
}

void SDCard::HandleCmd12BusyState(uint8_t& miso) {
    if (cmd12_busy_bytes > 0) {
        cmd12_busy_bytes--;
        miso = 0x00U;
    } else {
        miso = 0xFFU;
        state = State::IDLE;
        sd_status_byte = 0x00; // Clear status
    }
}

void SDCard::ProcessCommand() {
    uint8_t cmd = cmd_buffer.at(0) & 0x3FU;
    uint32_t arg = (static_cast<uint32_t>(cmd_buffer.at(1)) << 24) | (static_cast<uint32_t>(cmd_buffer.at(2)) << 16) |
                   (static_cast<uint32_t>(cmd_buffer.at(3)) << 8) | static_cast<uint32_t>(cmd_buffer.at(4));

    response_index = 0;
    response_buffer.clear();

    if (cmd == 0 || cmd == 8) {
        if (cmd_buffer.at(5) != CalculateCrc7(cmd_buffer)) {
            QueueResponse1(0x09U);  // R1: Idle + CRC error
            return;
        }
    }

    if (is_acmd) {
        HandleAcmd(cmd, arg);
    } else {
        HandleStandardCmd(cmd, arg);
    }
}

void SDCard::HandleAcmd(uint8_t cmd, uint32_t arg) {
    is_acmd = false;
    switch (cmd) {
        case 41:
            HandleAcmd41(arg);
            break;
        default:
            QueueResponse1(0x04U);  // R1: Illegal command
            break;
    }
}

void SDCard::HandleAcmd41(uint32_t arg) {
    if (acmd41_attempts < acmd41_target_attempts) {
        acmd41_attempts++;
        QueueResponse1(0x01U);  // R1: In Idle State
    } else {
        is_sdhc = ((arg >> 30) & 1U) != 0; // HCS bit
        QueueResponse1(0x00U);  // R1: Ready (no idle bit)
        is_initialized = true;
    }
}

void SDCard::HandleStandardCmd(uint8_t cmd, uint32_t arg) {
    switch (cmd) {
        case 0:
            HandleCmd0();
            break;
        case 8:
            HandleCmd8(arg);
            break;
        case 12:
            HandleCmd12();
            break;
        case 13:
            HandleCmd13();
            break;
        case 16:
            HandleCmd16();
            break;
        case 17:
            HandleCmd17(arg);
            break;
        case 18:
            HandleCmd18(arg);
            break;
        case 24:
            HandleCmd24(arg);
            break;
        case 25:
            HandleCmd25(arg);
            break;
        case 55:
            HandleCmd55();
            break;
        case 58:
            HandleCmd58();
            break;
        case 59:
            HandleCmd59(arg);
            break;
        default:
            QueueResponse1(is_initialized ? 0x04U : 0x05U);  // R1: Illegal command
            break;
    }
}

void SDCard::HandleCmd0() {
    QueueResponse1(0x01U);  // R1: In Idle State
    is_initialized = false;
    acmd41_attempts = 0;
    acmd41_target_attempts = static_cast<uint8_t>((std::rand() % 6) + 1);
    if (is_folder_mount && !currentPath.empty()) {
        SyncFromHost();
    }
}

void SDCard::HandleCmd8(uint32_t arg) {
    QueueResponse7(0x01U, arg & 0xFFFU);  // Echo back voltage pattern
}

void SDCard::HandleCmd12() {
    is_read_multiblock = false;
    cmd12_busy_bytes = 2; // Busy for a couple of bytes
    QueueResponse1(0x00U);
}

void SDCard::HandleCmd13() {
    QueueResponse2(is_initialized ? 0x00U : 0x01U, sd_status_byte); // R2 response
}

void SDCard::HandleCmd16() {
    // We always read/write 512 bytes
    QueueResponse1(is_initialized ? 0x00U : 0x01U);
}

bool SDCard::PrepareBlockTransfer(uint32_t arg, State nextState, bool isMultiblock, bool isWrite) {
    (void)nextState;
    if (!is_initialized) {
        QueueResponse1(0x05U);  // R1: Illegal command + In Idle State
        return false;
    }
    if (arg >= total_blocks && is_sdhc) {
        sd_status_byte |= 0x40U;
        QueueResponse1(0x40U); // R1: Parameter Error
        return false;
    }
    current_lba = arg;
    if (!is_sdhc) {
        current_lba /= 512U;
        if (current_lba >= total_blocks) {
            sd_status_byte |= 0x40U;
            QueueResponse1(0x40U);
            return false;
        }
    }
    if (isWrite) {
        is_write_multiblock = isMultiblock;
    } else {
        is_read_multiblock = isMultiblock;
        ReadBlockFromImage();
    }
    QueueResponse1(0x00U);  // R1: Success
    return true;
}

void SDCard::HandleCmd17(uint32_t arg) {
    PrepareBlockTransfer(arg, State::READ_PENDING, false, false);
}

void SDCard::HandleCmd18(uint32_t arg) {
    PrepareBlockTransfer(arg, State::READ_PENDING, true, false);
}

void SDCard::HandleCmd24(uint32_t arg) {
    PrepareBlockTransfer(arg, State::WRITE_DATA_TOKEN, false, true);
}

void SDCard::HandleCmd25(uint32_t arg) {
    PrepareBlockTransfer(arg, State::WRITE_DATA_TOKEN, true, true);
}

void SDCard::HandleCmd55() {
    QueueResponse1(is_initialized ? 0x00U : 0x01U);
    is_acmd = true;
}

void SDCard::HandleCmd58() {
    // Return OCR. Bit 30 (CCS) = is_sdhc ? 1 : 0
    uint32_t ocr = is_sdhc ? 0xC0FF8000U : 0x80FF8000U;
    QueueResponse3(is_initialized ? 0x00U : 0x01U, ocr);
}

void SDCard::HandleCmd59(uint32_t arg) {
    crc_enabled = (arg & 1U) != 0;
    QueueResponse1(is_initialized ? 0x00U : 0x01U);
}

void SDCard::QueueResponse1(uint8_t response1) {
    response_buffer.clear();
    response_buffer.push_back(response1);
    response_index = 0;
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse2(uint8_t response1, uint8_t response2) {
    response_buffer.clear();
    response_buffer.push_back(response1);
    response_buffer.push_back(response2);
    response_index = 0;
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse3(uint8_t response1, uint32_t response3) {
    response_buffer.clear();
    response_buffer.push_back(response1);
    response_buffer.push_back(static_cast<uint8_t>((response3 >> 24) & 0xFFU));
    response_buffer.push_back(static_cast<uint8_t>((response3 >> 16) & 0xFFU));
    response_buffer.push_back(static_cast<uint8_t>((response3 >> 8) & 0xFFU));
    response_buffer.push_back(static_cast<uint8_t>(response3 & 0xFFU));
    response_index = 0;
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse7(uint8_t response1, uint32_t response7) {
    QueueResponse3(response1, response7);
}

void SDCard::ReadBlockFromImage() {
    if (is_folder_mount) {
        if (current_lba < total_blocks && (static_cast<size_t>(current_lba) + 1) * 512 <= virtual_disk.size()) {
            std::memcpy(data_buffer.data(), virtual_disk.data() + (static_cast<size_t>(current_lba) * 512), 512);
        } else {
            data_buffer.fill(0xFFU);
        }
        return;
    }

    if (!mounted || !imageFile.is_open()) {
        data_buffer.fill(0xFFU);
        return;
    }

    imageFile.clear();  // Clear any EOF flags
    imageFile.seekg(static_cast<std::streamoff>(current_lba) * 512, std::ios::beg);
    ISerializable::Deserialize(imageFile, data_buffer);
}

void SDCard::WriteBlockToImage() {
    if (is_folder_mount) {
        if (is_read_only) {
            return;
        }
        if (current_lba < total_blocks && (static_cast<size_t>(current_lba) + 1) * 512 <= virtual_disk.size()) {
            std::memcpy(virtual_disk.data() + (static_cast<size_t>(current_lba) * 512), data_buffer.data(), 512);
            is_modified = true;
        }
        return;
    }

    if (!mounted || !imageFile.is_open() || is_read_only) {
        return;
    }

    imageFile.clear();
    imageFile.seekp(static_cast<std::streamoff>(current_lba) * 512, std::ios::beg);
    ISerializable::Serialize(imageFile, data_buffer);
    imageFile.flush();
    is_modified = true;
}

bool SDCard::SaveState(std::ostream& out) const {
    std::string verStr = "SD_VER:" + std::string(PROJECT_VERSION);
    ISerializable::Serialize(out, verStr);

    ISerializable::Serialize(out, currentPath);

    ISerializable::Serialize(out, mounted);
    ISerializable::Serialize(out, cs_active);
    ISerializable::Serialize(out, state);
    ISerializable::Serialize(out, last_miso);
    ISerializable::Serialize(out, cmd_buffer);
    ISerializable::Serialize(out, cmd_bytes_received);

    ISerializable::Serialize(out, response_buffer);

    ISerializable::Serialize(out, response_index);
    ISerializable::Serialize(out, data_buffer);
    ISerializable::Serialize(out, data_index);
    ISerializable::Serialize(out, current_lba);
    ISerializable::Serialize(out, is_acmd);
    ISerializable::Serialize(out, is_initialized);
    ISerializable::Serialize(out, is_sdhc);
    ISerializable::Serialize(out, is_read_multiblock);
    ISerializable::Serialize(out, is_write_multiblock);
    ISerializable::Serialize(out, sd_status_byte);

    ISerializable::Serialize(out, is_read_only);
    ISerializable::Serialize(out, warmup_bytes);
    ISerializable::Serialize(out, acmd41_attempts);
    ISerializable::Serialize(out, acmd41_target_attempts);
    ISerializable::Serialize(out, write_busy_bytes);
    ISerializable::Serialize(out, cmd12_busy_bytes);
    
    ISerializable::Serialize(out, read_delay_bytes);
    ISerializable::Serialize(out, crc_enabled);
    ISerializable::Serialize(out, read_latency_enabled);
    ISerializable::Serialize(out, current_crc);
    ISerializable::Serialize(out, received_crc);
    ISerializable::Serialize(out, total_blocks);

    return out.good();
}

bool SDCard::LoadState(std::istream& inStream) {
    std::string firstStr;
    ISerializable::Deserialize(inStream, firstStr);
    
    bool is_old_format = false;
    std::string currentPath;
    if (firstStr.find("SD_VER:") == 0) {
        ISerializable::Deserialize(inStream, currentPath);
    } else {
        is_old_format = true;
        currentPath = firstStr;
    }

    ISerializable::Deserialize(inStream, mounted);
    ISerializable::Deserialize(inStream, cs_active);
    ISerializable::Deserialize(inStream, state);
    ISerializable::Deserialize(inStream, last_miso);
    ISerializable::Deserialize(inStream, cmd_buffer);
    ISerializable::Deserialize(inStream, cmd_bytes_received);

    ISerializable::Deserialize(inStream, response_buffer);

    ISerializable::Deserialize(inStream, response_index);
    ISerializable::Deserialize(inStream, data_buffer);
    ISerializable::Deserialize(inStream, data_index);
    ISerializable::Deserialize(inStream, current_lba);
    ISerializable::Deserialize(inStream, is_acmd);
    ISerializable::Deserialize(inStream, is_initialized);
    ISerializable::Deserialize(inStream, is_sdhc);

    if (is_old_format) {
        is_read_only = false;
        warmup_bytes = 10;
        acmd41_attempts = 0;
        acmd41_target_attempts = 1;
        write_busy_bytes = 0;
        cmd12_busy_bytes = 0;
        read_delay_bytes = 0;
        crc_enabled = false;
        read_latency_enabled = false;
        current_crc = 0;
        received_crc = 0;
        is_read_multiblock = false;
        is_write_multiblock = false;
        total_blocks = 0;
        sd_status_byte = 0x00;
    } else {
        ISerializable::Deserialize(inStream, is_read_multiblock);
        ISerializable::Deserialize(inStream, is_write_multiblock);
        ISerializable::Deserialize(inStream, sd_status_byte);
        ISerializable::Deserialize(inStream, is_read_only);
        ISerializable::Deserialize(inStream, warmup_bytes);
        ISerializable::Deserialize(inStream, acmd41_attempts);
        ISerializable::Deserialize(inStream, acmd41_target_attempts);
        ISerializable::Deserialize(inStream, write_busy_bytes);
        ISerializable::Deserialize(inStream, cmd12_busy_bytes);
        ISerializable::Deserialize(inStream, read_delay_bytes);
        ISerializable::Deserialize(inStream, crc_enabled);
        ISerializable::Deserialize(inStream, read_latency_enabled);
        ISerializable::Deserialize(inStream, current_crc);
        ISerializable::Deserialize(inStream, received_crc);
        ISerializable::Deserialize(inStream, total_blocks);
    }

    if (imageFile.is_open()) {
        imageFile.close();
    }
    if (mounted && !currentPath.empty()) {
        if (std::filesystem::is_directory(PathFromUtf8(currentPath))) {
            if (!(is_folder_mount && this->currentPath == currentPath && !virtual_disk.empty())) {
                auto saved_mounted = mounted;
                auto saved_cs_active = cs_active;
                auto saved_state = state;
                auto saved_last_miso = last_miso;
                auto saved_cmd_buffer = cmd_buffer;
                auto saved_cmd_bytes_received = cmd_bytes_received;
                auto saved_response_buffer = response_buffer;
                auto saved_response_index = response_index;
                auto saved_data_buffer = data_buffer;
                auto saved_data_index = data_index;
                auto saved_current_lba = current_lba;
                auto saved_is_acmd = is_acmd;
                auto saved_is_initialized = is_initialized;
                auto saved_is_sdhc = is_sdhc;
                auto saved_is_read_multiblock = is_read_multiblock;
                auto saved_is_write_multiblock = is_write_multiblock;
                auto saved_sd_status_byte = sd_status_byte;
                auto saved_is_read_only = is_read_only;
                auto saved_warmup_bytes = warmup_bytes;
                auto saved_acmd41_attempts = acmd41_attempts;
                auto saved_acmd41_target_attempts = acmd41_target_attempts;
                auto saved_write_busy_bytes = write_busy_bytes;
                auto saved_cmd12_busy_bytes = cmd12_busy_bytes;
                auto saved_read_delay_bytes = read_delay_bytes;
                auto saved_crc_enabled = crc_enabled;
                auto saved_read_latency_enabled = read_latency_enabled;
                auto saved_current_crc = current_crc;
                auto saved_received_crc = received_crc;
                auto saved_total_blocks = total_blocks;

                if (MountFolder(currentPath)) {
                    mounted = saved_mounted;
                    cs_active = saved_cs_active;
                    state = saved_state;
                    last_miso = saved_last_miso;
                    cmd_buffer = saved_cmd_buffer;
                    cmd_bytes_received = saved_cmd_bytes_received;
                    response_buffer = std::move(saved_response_buffer);
                    response_index = saved_response_index;
                    data_buffer = saved_data_buffer;
                    data_index = saved_data_index;
                    current_lba = saved_current_lba;
                    is_acmd = saved_is_acmd;
                    is_initialized = saved_is_initialized;
                    is_sdhc = saved_is_sdhc;
                    is_read_multiblock = saved_is_read_multiblock;
                    is_write_multiblock = saved_is_write_multiblock;
                    sd_status_byte = saved_sd_status_byte;
                    is_read_only = saved_is_read_only;
                    warmup_bytes = saved_warmup_bytes;
                    acmd41_attempts = saved_acmd41_attempts;
                    acmd41_target_attempts = saved_acmd41_target_attempts;
                    write_busy_bytes = saved_write_busy_bytes;
                    cmd12_busy_bytes = saved_cmd12_busy_bytes;
                    read_delay_bytes = saved_read_delay_bytes;
                    crc_enabled = saved_crc_enabled;
                    read_latency_enabled = saved_read_latency_enabled;
                    current_crc = saved_current_crc;
                    received_crc = saved_received_crc;
                    total_blocks = saved_total_blocks;
                } else {
                    mounted = false;
                }
            }
        } else {
            imageFile.open(PathFromUtf8(currentPath), std::ios::in | std::ios::out | std::ios::binary);
            if (!imageFile.is_open()) {
                imageFile.clear();
                imageFile.open(PathFromUtf8(currentPath), std::ios::in | std::ios::binary);
                if (imageFile.is_open()) {
                    is_read_only = true;
                }
            } else {
                is_read_only = false;
            }
            if (imageFile.is_open()) {
                imageFile.seekg(0, std::ios::end);
                total_blocks = static_cast<uint32_t>(imageFile.tellg() / 512);
                imageFile.seekg(0, std::ios::beg);
            } else {
                mounted = false;
            }
        }
    }
    this->currentPath = currentPath;

    return inStream.good();
}

uint8_t SDCard::CalculateCrc7(const std::array<std::uint8_t, 6>& buffer) const {
    uint8_t crc = 0;
    for (int i = 0; i < 5; i++) {
        crc ^= buffer.at(static_cast<size_t>(i));
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x80U) != 0) {
                crc ^= 0x89U;
            }
            crc = static_cast<uint8_t>(crc << 1);
        }
    }
    return static_cast<uint8_t>(crc | 1U);
}

uint16_t SDCard::CalculateCrc16(const std::array<std::uint8_t, 512>& buffer) const {
    uint16_t crc = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        crc ^= static_cast<uint16_t>(buffer.at(i)) << 8;
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x8000U) != 0) {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021U);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

}  // namespace Hardware
