#include "Hardware/Core/Emulator.h"

#include <picosha2.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace Core {

bool Emulator::SaveState(const std::string& filename) {
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);

    std::stringstream stateStream;
    for (auto* component : components) {
        if (!component->SaveState(stateStream)) {
            return false;
        }
    }

    stateStream.write(reinterpret_cast<const char*>(&baudDelay), sizeof(baudDelay));  // NOLINT

    bool gpuE = gpuEnabled;
    stateStream.write(reinterpret_cast<const char*>(&gpuE), sizeof(gpuE));  // NOLINT

    int tIPS = targetIPS.load();
    stateStream.write(reinterpret_cast<const char*>(&tIPS), sizeof(tIPS));  // NOLINT

    size_t qSize = inputBuffer.size();
    stateStream.write(reinterpret_cast<const char*>(&qSize), sizeof(qSize));  // NOLINT
    for (char chr : inputBuffer) {
        stateStream.write(&chr, 1);
    }

    size_t binPathLen = currentBinPath.length();
    stateStream.write(reinterpret_cast<const char*>(&binPathLen), sizeof(binPathLen));  // NOLINT
    stateStream.write(currentBinPath.c_str(), static_cast<std::streamsize>(binPathLen));

    bool autoReload = autoReloadRequested.load();
    stateStream.write(reinterpret_cast<const char*>(&autoReload), sizeof(autoReload));  // NOLINT

    std::string payload = stateStream.str();
    std::string payloadHash = picosha2::hash256_hex_string(payload);

    std::string version = PROJECT_VERSION;
    auto versionLen = static_cast<uint32_t>(version.length());
    std::array<char, 12> magic{"SIM65C02SST"};  // NOLINT

    // Metadata hash (Magic + Version)
    std::string metadata = std::string(magic.data()) + version;
    std::string metadataHash = picosha2::hash256_hex_string(metadata);

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    out.write(magic.data(), magic.size() - 1);
    out.write(reinterpret_cast<const char*>(&versionLen), sizeof(versionLen));  // NOLINT
    out.write(version.c_str(), static_cast<std::streamsize>(versionLen));
    out.write(payload.c_str(), static_cast<std::streamsize>(payload.size()));
    out.write(payloadHash.c_str(), static_cast<std::streamsize>(payloadHash.size()));
    out.write(metadataHash.c_str(), static_cast<std::streamsize>(metadataHash.size()));

    return out.good();
}

bool Emulator::LoadState(const std::string& filename, bool forceLoad) {
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);
    SetupHardware();

    std::ifstream inFile(filename, std::ios::binary | std::ios::ate);

    if (!inFile.is_open()) {
        return false;
    }

    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    std::array<char, 12> magic{"SIM65C02SST"};  // NOLINT
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
    inFile.read(reinterpret_cast<char*>(&versionLen), sizeof(versionLen));  // NOLINT
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
        LoadInternalState(stateStream);
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
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);
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

bool Emulator::CanRewind() const { return !rewindBuffer.empty(); }

bool Emulator::LoadComponentsState(std::istream& stateStream) {
    for (auto* component : components) {
        if (!component->LoadState(stateStream)) {
            return false;
        }
    }
    return true;
}

void Emulator::LoadInternalState(std::istream& stateStream) {
    stateStream.read(reinterpret_cast<char*>(&baudDelay), sizeof(baudDelay));  // NOLINT

    bool gpuE = false;
    stateStream.read(reinterpret_cast<char*>(&gpuE), sizeof(gpuE));  // NOLINT
    gpuEnabled = gpuE;

    int tIPS = 0;
    stateStream.read(reinterpret_cast<char*>(&tIPS), sizeof(tIPS));  // NOLINT
    targetIPS.store(tIPS);

    size_t qSize = 0;
    stateStream.read(reinterpret_cast<char*>(&qSize), sizeof(qSize));  // NOLINT
    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.clear();
    for (size_t i = 0; i < qSize; ++i) {
        char chr = '\0';
        stateStream.read(&chr, 1);
        inputBuffer.push_back(chr);
    }
    hasInput.store(!inputBuffer.empty());

    size_t binPathLen = 0;
    stateStream.read(reinterpret_cast<char*>(&binPathLen), sizeof(binPathLen));  // NOLINT
    currentBinPath.assign(binPathLen, '\0');
    stateStream.read(currentBinPath.data(), static_cast<std::streamsize>(binPathLen));

    bool autoReload = false;
    stateStream.read(reinterpret_cast<char*>(&autoReload), sizeof(autoReload));  // NOLINT
    autoReloadRequested.store(autoReload);
}

void Emulator::SaveStateToBuffer() {
    std::stringstream stateStream;
    for (auto* component : components) {
        if (!component->SaveState(stateStream)) {
            return;
        }
    }

    stateStream.write(reinterpret_cast<const char*>(&baudDelay), sizeof(baudDelay));  // NOLINT
    bool gpuE = gpuEnabled;
    stateStream.write(reinterpret_cast<const char*>(&gpuE), sizeof(gpuE));  // NOLINT
    int tIPS = targetIPS.load();
    stateStream.write(reinterpret_cast<const char*>(&tIPS), sizeof(tIPS));  // NOLINT

    size_t qSize = inputBuffer.size();
    stateStream.write(reinterpret_cast<const char*>(&qSize), sizeof(qSize));  // NOLINT
    for (char chr : inputBuffer) {
        stateStream.write(&chr, 1);
    }

    size_t binPathLen = currentBinPath.length();
    stateStream.write(reinterpret_cast<const char*>(&binPathLen), sizeof(binPathLen));  // NOLINT
    stateStream.write(currentBinPath.data(), static_cast<std::streamsize>(binPathLen));
    bool autoReload = autoReloadRequested.load();
    stateStream.write(reinterpret_cast<const char*>(&autoReload), sizeof(autoReload));  // NOLINT

    rewindBuffer.push_back(stateStream.str());
    if (rewindBuffer.size() > MAX_REWIND_STATES) {
        rewindBuffer.pop_front();
    }
}

}  // namespace Core
