#include "Hardware/Core/Emulator.h"
#include "Hardware/Core/ISerializable.h"
#include <picosha2.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace Core {

bool Emulator::SaveState(const std::string& filename) {
    WaitUntilSafeToMutate();

    std::stringstream stateStream;
    for (auto* component : components) {
        if (!component->SaveState(stateStream)) {
            return false;
        }
    }

    Hardware::ISerializable::Serialize(stateStream, baudDelay);

    bool gpuE = gpuEnabled;
    Hardware::ISerializable::Serialize(stateStream, gpuE);

    int tIPS = targetIPS.load();
    Hardware::ISerializable::Serialize(stateStream, tIPS);

    {
        std::lock_guard<std::mutex> bufferLock(bufferMutex);
        auto qSize = static_cast<uint32_t>(inputBuffer.size());
        Hardware::ISerializable::Serialize(stateStream, qSize);
        for (char chr : inputBuffer) {
            stateStream.write(&chr, 1);
        }
    }


    auto binPathLen = static_cast<uint32_t>(currentBinPath.length());
    Hardware::ISerializable::Serialize(stateStream, binPathLen);
    stateStream.write(currentBinPath.c_str(), static_cast<std::streamsize>(binPathLen));

    bool autoReload = autoReloadRequested.load();
    Hardware::ISerializable::Serialize(stateStream, autoReload);

    std::string payload = stateStream.str();
    std::string payloadHash = picosha2::hash256_hex_string(payload);

    std::string version = PROJECT_VERSION;
    auto versionLen = static_cast<uint32_t>(version.length());
    std::array<char, 12> magic{"SIM65C02SST"};

    // Metadata hash (Magic + Version)
    std::string metadata = std::string(magic.data()) + version;
    std::string metadataHash = picosha2::hash256_hex_string(metadata);

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    out.write(magic.data(), magic.size() - 1);
    Hardware::ISerializable::Serialize(out, versionLen);
    out.write(version.c_str(), static_cast<std::streamsize>(versionLen));
    out.write(payload.c_str(), static_cast<std::streamsize>(payload.size()));
    out.write(payloadHash.c_str(), static_cast<std::streamsize>(payloadHash.size()));
    out.write(metadataHash.c_str(), static_cast<std::streamsize>(metadataHash.size()));

    return out.good();
}

bool Emulator::LoadState(const std::string& filename, bool forceLoad) {
    WaitUntilSafeToMutate();
    SetupHardware();

    std::ifstream inFile(filename, std::ios::binary | std::ios::ate);

    if (!inFile.is_open()) {
        return false;
    }

    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    std::array<char, 12> magic{"SIM65C02SST"};
    size_t magicLen = magic.size() - 1;
    size_t hashLen = 64;

    if (size < (std::streamsize)(magicLen + sizeof(uint32_t) + (hashLen * 2))) {
        return false;
    }

    std::array<char, 16> fileMagic{};
    inFile.read(fileMagic.data(), static_cast<std::streamsize>(magicLen));
    if (strncmp(fileMagic.data(), magic.data(), magicLen) != 0) {
        return false;
    }

    uint32_t versionLen = 0;
    Hardware::ISerializable::Deserialize(inFile, versionLen);
    if (versionLen > 64) {
        return false;
    }
    lastLoadVersion.assign(versionLen, '\0');
    inFile.read(lastLoadVersion.data(), static_cast<std::streamsize>(versionLen));

    // Calculate metadata hash for verification (Magic + Version)
    std::string metadata = std::string(magic.data()) + lastLoadVersion;
    std::string computedMetadataHash = picosha2::hash256_hex_string(metadata);

    size_t payloadSize = size - (magicLen + sizeof(versionLen) + versionLen + (hashLen * 2));
    std::string payload(payloadSize, '\0');
    inFile.read(payload.data(), static_cast<std::streamsize>(payloadSize));

    std::array<char, 64> filePayloadHash{};
    inFile.read(filePayloadHash.data(), static_cast<std::streamsize>(hashLen));

    std::array<char, 64> fileMetadataHash{};
    inFile.read(fileMetadataHash.data(), static_cast<std::streamsize>(hashLen));

    lastLoadResult = SavestateLoadResult::Success;

    // Verify Metadata Hash
    if (strncmp(fileMetadataHash.data(), computedMetadataHash.c_str(), hashLen) != 0) {
        std::cerr << "Error: Savestate metadata corruption detected!\n";
        lastLoadResult = SavestateLoadResult::GenericError;
        if (!forceLoad) {
            return false;
        }
    }

    // Verify Payload Hash
    std::string computedPayloadHash = picosha2::hash256_hex_string(payload);
    if (strncmp(filePayloadHash.data(), computedPayloadHash.c_str(), hashLen) != 0) {
        lastLoadResult = SavestateLoadResult::HashMismatch;
        std::cerr << "Warning: Savestate payload hash mismatch!\n";
    }

    // Check Version
    if (lastLoadVersion != PROJECT_VERSION) {
        if (lastLoadResult == SavestateLoadResult::Success) {
            lastLoadResult = SavestateLoadResult::VersionMismatch;
        }
        std::cerr << "Warning: Savestate version mismatch! File: " << lastLoadVersion << " Current: " << PROJECT_VERSION
                  << '\n';
    }

    std::stringstream stateStream(payload);

    bool structuralSuccess = LoadComponentsState(stateStream);

    if (structuralSuccess) {
        structuralSuccess = LoadInternalState(stateStream);
    }

    if (!structuralSuccess || (!stateStream.good() && !stateStream.eof())) {
        lastLoadResult = SavestateLoadResult::StructuralError;
        if (!forceLoad) {
            return false;
        }
    }

    halted = false;
    return true;
}

void Emulator::Rewind() {
    WaitUntilSafeToMutate();
    if (rewindBuffer.empty()) {
        return;
    }

    std::string statePayload = rewindBuffer.back();
    rewindBuffer.pop_back();

    std::stringstream stateStream(statePayload);
    if (LoadComponentsState(stateStream)) {
        LoadInternalState(stateStream);
    }
}

bool Emulator::CanRewind() const { 
    return !rewindBuffer.empty(); 
}

bool Emulator::LoadComponentsState(std::istream& stateStream) {
    for (auto* component : components) {
        if (!component->LoadState(stateStream)) {
            return false;
        }
    }
    return true;
}

bool Emulator::LoadInternalState(std::istream& stateStream) {
    Hardware::ISerializable::Deserialize(stateStream, baudDelay);

    bool gpuE = false;
    Hardware::ISerializable::Deserialize(stateStream, gpuE);
    gpuEnabled = gpuE;

    int tIPS = 0;
    Hardware::ISerializable::Deserialize(stateStream, tIPS);
    targetIPS.store(tIPS);

    uint32_t qSize = 0;
    Hardware::ISerializable::Deserialize(stateStream, qSize);
    if (qSize > 10ULL * 1024 * 1024) { return false; }
    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.clear();
    for (size_t i = 0; i < qSize; ++i) {
        char chr = '\0';
        stateStream.read(&chr, 1);
        inputBuffer.push_back(chr);
    }
    hasInput.store(!inputBuffer.empty());


    uint32_t binPathLen = 0;
    Hardware::ISerializable::Deserialize(stateStream, binPathLen);
    if (binPathLen > 4096) { return false; }
    currentBinPath.assign(binPathLen, '\0');
    stateStream.read(currentBinPath.data(), static_cast<std::streamsize>(binPathLen));

    bool autoReload = false;
    Hardware::ISerializable::Deserialize(stateStream, autoReload);
    autoReloadRequested.store(autoReload);
    return true;
}

void Emulator::SaveStateToBuffer() {
    std::stringstream stateStream;
    for (auto* component : components) {
        if (!component->SaveState(stateStream)) {
            return;
        }
    }

    Hardware::ISerializable::Serialize(stateStream, baudDelay);
    bool gpuE = gpuEnabled;
    Hardware::ISerializable::Serialize(stateStream, gpuE);
    int tIPS = targetIPS.load();
    Hardware::ISerializable::Serialize(stateStream, tIPS);

    {
        std::lock_guard<std::mutex> bufferLock(bufferMutex);
        auto qSize = static_cast<uint32_t>(inputBuffer.size());
        Hardware::ISerializable::Serialize(stateStream, qSize);
        for (char chr : inputBuffer) {
            stateStream.write(&chr, 1);
        }
    }

    auto binPathLen = static_cast<uint32_t>(currentBinPath.length());
    Hardware::ISerializable::Serialize(stateStream, binPathLen);
    stateStream.write(currentBinPath.data(), static_cast<std::streamsize>(binPathLen));
    bool autoReload = autoReloadRequested.load();
    Hardware::ISerializable::Serialize(stateStream, autoReload);

    rewindBuffer.push_back(stateStream.str());
    if (rewindBuffer.size() > MAX_REWIND_STATES) {
        rewindBuffer.pop_front();
    }
}

}  // namespace Core
