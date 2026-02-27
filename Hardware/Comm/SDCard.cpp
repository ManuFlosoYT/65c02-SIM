#include "Hardware/Comm/SDCard.h"

#include <array>
#include <cstring>
#include <iostream>

namespace Hardware {

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
}

bool SDCard::Mount(const std::string& imagePath) {
    if (mounted) {
        Unmount();
    }

    imageFile.open(imagePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!imageFile.is_open()) {
        // Try opening read-only if read-write failed
        imageFile.clear();
        imageFile.open(imagePath, std::ios::in | std::ios::binary);
    }
    if (!imageFile.is_open()) {
        // Try creating it if it doesn't exist
        imageFile.clear();
        imageFile.open(imagePath, std::ios::out | std::ios::binary);
        if (imageFile.is_open()) {
            imageFile.close();
            imageFile.open(imagePath, std::ios::in | std::ios::out | std::ios::binary);
        }
    }

    if (imageFile.is_open()) {
        currentPath = imagePath;
        mounted = true;
        Reset();
        return true;
    }
    return false;
}

void SDCard::Unmount() {
    if (imageFile.is_open()) {
        imageFile.close();
    }
    mounted = false;
}

void SDCard::SetCS(bool csVal) {
    // CS is active low
    cs_active = !csVal;

    if (!cs_active) {
        // When CS goes high, the transaction is canceled or done
        // We might want to clear output buffer to 0xFF, but SPI keeps outputting last/0xFF
        state = State::IDLE;
        cmd_bytes_received = 0;
        response_buffer.clear();
    }
}

uint8_t SDCard::TransferByte(uint8_t mosi) {
    if (!mounted || !cs_active) {
        return 0xFFU;  // When deselected or unmounted, MISO is floating (pulled up to 0xFF)
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
        case State::READ_DATA_TOKEN:
            HandleReadDataTokenState(miso);
            break;
        case State::READ_DATA_BLOCK:
            HandleReadDataBlockState(miso);
            break;
        case State::READ_DATA_CRC:
            HandleReadDataCrcState(miso);
            break;
        case State::WRITE_DATA_TOKEN:
            HandleWriteDataTokenState(mosi);
            break;
        case State::WRITE_DATA_BLOCK:
            HandleWriteDataBlockState(mosi);
            break;
        case State::WRITE_DATA_CRC:
            HandleWriteDataCrcState(miso);
            break;
        case State::WRITE_BUSY:
            HandleWriteBusyState(miso);
            break;
        default:
            state = State::IDLE;
            break;
    }

    return miso;
}

void SDCard::HandleIdleState(uint8_t mosi) {
    if ((mosi & 0xC0U) == 0x40U) {  // Command starts with 01
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
            // If we just finished sending response for CMD24, we must wait for data token
            uint8_t last_cmd = cmd_buffer[0] & 0x3FU;
            if (last_cmd == 24U && !is_acmd) {
                state = State::WRITE_DATA_TOKEN;
            } else {
                state = State::IDLE;
            }
        }
    } else {
        state = State::IDLE;
    }
}

void SDCard::HandleReadDataTokenState(uint8_t& miso) {
    // Output token 0xFE before data block
    miso = 0xFEU;
    data_index = 0;
    state = State::READ_DATA_BLOCK;
}

void SDCard::HandleReadDataBlockState(uint8_t& miso) {
    miso = data_buffer.at(static_cast<size_t>(data_index++));
    if (data_index == 512) {
        state = State::READ_DATA_CRC;
        data_index = 0;
    }
}

void SDCard::HandleReadDataCrcState(uint8_t& miso) {
    miso = 0xFFU;  // Dummy CRC
    data_index++;
    if (data_index == 2) {
        state = State::IDLE;
    }
}

void SDCard::HandleWriteDataTokenState(uint8_t mosi) {
    if (mosi == 0xFEU) {  // Start block token
        data_index = 0;
        state = State::WRITE_DATA_BLOCK;
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

void SDCard::HandleWriteDataCrcState(uint8_t& miso) {
    // Ignore CRC bytes
    data_index++;
    if (data_index == 2) {
        WriteBlockToImage();
        // Send Data Response:
        // xxx00101b = 0x05 -> Accepted
        miso = 0x05U;
        state = State::WRITE_BUSY;
    }
}

void SDCard::HandleWriteBusyState(uint8_t& miso) {
    // Simulate immediately finishing writing (output 0xFF instead of 0x00 busy)
    miso = 0xFFU;
    state = State::IDLE;
}

void SDCard::ProcessCommand() {
    uint8_t cmd = cmd_buffer.at(0) & 0x3FU;
    uint32_t arg = (static_cast<uint32_t>(cmd_buffer.at(1)) << 24) | (static_cast<uint32_t>(cmd_buffer.at(2)) << 16) |
                   (static_cast<uint32_t>(cmd_buffer.at(3)) << 8) | static_cast<uint32_t>(cmd_buffer.at(4));

    response_index = 0;
    response_buffer.clear();

    if (is_acmd) {
        HandleAcmd(cmd, arg);
    } else {
        HandleStandardCmd(cmd, arg);
    }
}

void SDCard::HandleAcmd(uint8_t cmd, uint32_t /*arg*/) {
    is_acmd = false;
    switch (cmd) {
        case 41:
            HandleAcmd41();
            break;
        default:
            QueueResponse1(0x04U);  // R1: Illegal command
            break;
    }
}

void SDCard::HandleAcmd41() {
    QueueResponse1(0x00U);  // R1: Ready (no idle bit)
    is_initialized = true;
}

void SDCard::HandleStandardCmd(uint8_t cmd, uint32_t arg) {
    switch (cmd) {
        case 0:
            HandleCmd0();
            break;
        case 8:
            HandleCmd8(arg);
            break;
        case 16:
            HandleCmd16();
            break;
        case 17:
            HandleCmd17(arg);
            break;
        case 24:
            HandleCmd24(arg);
            break;
        case 55:
            HandleCmd55();
            break;
        case 58:
            HandleCmd58();
            break;
        default:
            QueueResponse1(0x04U);  // R1: Illegal command
            break;
    }
}

void SDCard::HandleCmd0() {
    QueueResponse1(0x01U);  // R1: In Idle State
    is_initialized = false;
}

void SDCard::HandleCmd8(uint32_t arg) {
    QueueResponse7(0x01U, arg & 0xFFFU);  // Echo back voltage pattern
}

void SDCard::HandleCmd16() {
    // We always read/write 512 bytes
    QueueResponse1(is_initialized ? 0x00U : 0x01U);
}

void SDCard::HandleCmd17(uint32_t arg) {
    current_lba = arg;
    if (!is_sdhc) {
        current_lba /= 512U;
    }
    ReadBlockFromImage();
    QueueResponse1(0x00U);             // R1: Success
    response_buffer.push_back(0xFFU);  // Delay
    response_buffer.push_back(0xFEU);  // Data token
    for (int i = 0; i < 512; i++) {
        response_buffer.push_back(data_buffer.at(static_cast<size_t>(i)));
    }
    response_buffer.push_back(0xFFU);  // CRC high
    response_buffer.push_back(0xFFU);  // CRC low
    state = State::WAIT_RESPONSE;
}

void SDCard::HandleCmd24(uint32_t arg) {
    current_lba = arg;
    if (!is_sdhc) {
        current_lba /= 512U;
    }
    QueueResponse1(0x00U);  // R1: Success
}

void SDCard::HandleCmd55() {
    QueueResponse1(is_initialized ? 0x00U : 0x01U);
    is_acmd = true;
}

void SDCard::HandleCmd58() {
    // Return OCR. Bit 30 (CCS) = 1 (SDHC)
    QueueResponse3(is_initialized ? 0x00U : 0x01U, 0xC0FF8000U);
}

void SDCard::QueueResponse1(uint8_t response1) {
    response_buffer.push_back(response1);
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse2(uint8_t response1, uint8_t response2) {
    response_buffer.push_back(response1);
    response_buffer.push_back(response2);
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse3(uint8_t response1, uint32_t response3) {
    response_buffer.push_back(response1);
    response_buffer.push_back((response3 >> 24) & 0xFF);
    response_buffer.push_back((response3 >> 16) & 0xFF);
    response_buffer.push_back((response3 >> 8) & 0xFF);
    response_buffer.push_back(response3 & 0xFF);
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse7(uint8_t response1, uint32_t response7) {
    QueueResponse3(response1, response7);  // R7 has same format as R3
}

namespace {

template <typename T>
void PodWrite(std::ostream& outStream, const T& val) {
    outStream.write(reinterpret_cast<const char*>(&val), static_cast<std::streamsize>(sizeof(T)));  // NOLINT
}

template <typename T>
void PodRead(std::istream& inStream, T& val) {
    inStream.read(reinterpret_cast<char*>(&val), static_cast<std::streamsize>(sizeof(T)));  // NOLINT
}

void BufferWrite(std::ostream& outStream, const void* bufferData, size_t bufferSize) {
    outStream.write(reinterpret_cast<const char*>(bufferData), static_cast<std::streamsize>(bufferSize));  // NOLINT
}

void BufferRead(std::istream& inStream, void* bufferData, size_t bufferSize) {
    inStream.read(reinterpret_cast<char*>(bufferData), static_cast<std::streamsize>(bufferSize));  // NOLINT
}

}  // namespace

void SDCard::ReadBlockFromImage() {
    if (!mounted || !imageFile.is_open()) {
        data_buffer.fill(0xFFU);
        return;
    }
    imageFile.clear();  // Clear any EOF flags
    imageFile.seekg(static_cast<std::streamoff>(current_lba) * 512, std::ios::beg);
    BufferRead(imageFile, data_buffer.data(), 512);
}

void SDCard::WriteBlockToImage() {
    if (!mounted || !imageFile.is_open()) {
        return;
    }
    imageFile.clear();
    imageFile.seekp(static_cast<std::streamoff>(current_lba) * 512, std::ios::beg);
    BufferWrite(imageFile, data_buffer.data(), 512);
    imageFile.flush();
}

bool SDCard::SaveState(std::ostream& out) const {
    size_t pathLen = currentPath.length();
    PodWrite(out, pathLen);
    out.write(currentPath.c_str(), static_cast<std::streamsize>(pathLen));

    PodWrite(out, mounted);
    PodWrite(out, cs_active);
    PodWrite(out, state);
    BufferWrite(out, cmd_buffer.data(), sizeof(cmd_buffer));
    PodWrite(out, cmd_bytes_received);

    size_t respSize = response_buffer.size();
    PodWrite(out, respSize);
    if (respSize > 0) {
        BufferWrite(out, response_buffer.data(), respSize);
    }

    PodWrite(out, response_index);
    BufferWrite(out, data_buffer.data(), sizeof(data_buffer));
    PodWrite(out, data_index);
    PodWrite(out, current_lba);
    PodWrite(out, is_acmd);
    PodWrite(out, is_initialized);
    PodWrite(out, is_sdhc);

    return out.good();
}

bool SDCard::LoadState(std::istream& inStream) {
    size_t pathLen = 0;
    PodRead(inStream, pathLen);
    currentPath.resize(pathLen);
    inStream.read(currentPath.data(), static_cast<std::streamsize>(pathLen));

    PodRead(inStream, mounted);
    PodRead(inStream, cs_active);
    PodRead(inStream, state);
    BufferRead(inStream, cmd_buffer.data(), sizeof(cmd_buffer));
    PodRead(inStream, cmd_bytes_received);

    size_t respSize = 0;
    PodRead(inStream, respSize);
    response_buffer.resize(respSize);
    if (respSize > 0) {
        BufferRead(inStream, response_buffer.data(), respSize);
    }

    PodRead(inStream, response_index);
    BufferRead(inStream, data_buffer.data(), sizeof(data_buffer));
    PodRead(inStream, data_index);
    PodRead(inStream, current_lba);
    PodRead(inStream, is_acmd);
    PodRead(inStream, is_initialized);
    PodRead(inStream, is_sdhc);

    if (mounted) {
        Mount(currentPath);
    }

    return inStream.good();
}

}  // namespace Hardware
