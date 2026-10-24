#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Core/ISerializable.h"
#include <picosha2.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace Core {

namespace {

SnapshotDelta CreateXORDiff(const std::string& oldState, const std::string& newState, uint64_t cycles,
                            uint64_t instructions) {
    SnapshotDelta delta;
    delta.totalCycles = cycles;
    delta.totalInstructions = instructions;

    if (oldState.empty() || oldState.size() != newState.size()) {
        delta.isKeyframe = true;
        delta.fullState = oldState;
        return delta;
    }

    const size_t len = oldState.size();
    size_t i = 0;
    size_t totalDiffBytes = 0;

    while (i < len) {
        if (oldState.at(i) == newState.at(i)) {
            ++i;
            continue;
        }
        size_t start = i;
        while (i < len && oldState.at(i) != newState.at(i)) {
            ++i;
        }

        SnapshotDelta::Chunk chunk;
        chunk.offset = static_cast<uint32_t>(start);
        chunk.xorData.resize(i - start);
        for (size_t j = 0; j < chunk.xorData.size(); ++j) {
            chunk.xorData.at(j) = static_cast<uint8_t>(oldState.at(start + j) ^ newState.at(start + j));
        }
        totalDiffBytes += chunk.xorData.size();
        delta.chunks.push_back(std::move(chunk));
    }

    if (totalDiffBytes > (len * 4) / 10) {
        delta.chunks.clear();
        delta.isKeyframe = true;
        delta.fullState = oldState;
    }

    return delta;
}

}  // namespace

bool Emulator::SaveComponentsState(std::ostream& stateStream) {
    for (auto* component : components) {
        if (!component->SaveState(stateStream)) {
            return false;
        }
    }
    return true;
}

void Emulator::SaveInternalState(std::ostream& stateStream) {
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
}

bool Emulator::SaveState(const std::string& filename) {
    WaitUntilSafeToMutate();

    std::stringstream stateStream;
    if (!SaveComponentsState(stateStream)) {
        return false;
    }
    SaveInternalState(stateStream);

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
    if (size < 0) {
        return false;
    }
    inFile.seekg(0, std::ios::beg);

    std::array<char, 12> magic{"SIM65C02SST"};
    size_t magicLen = magic.size() - 1;
    size_t hashLen = 64;

    if (size < static_cast<std::streamsize>(magicLen + sizeof(uint32_t) + (hashLen * 2))) {
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

    const size_t headerFooterSize = magicLen + sizeof(versionLen) + versionLen + (hashLen * 2);
    if (static_cast<size_t>(size) < headerFooterSize) {
        return false;
    }
    size_t payloadSize = static_cast<size_t>(size) - headerFooterSize;
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
    pendingInterruptAny.store(pendingIRQ.load() || pendingNMI.load() || via.isIRQAsserted() || acia.HasIRQ(), std::memory_order_relaxed);
    return true;
}

void Emulator::Rewind() {
    WaitUntilSafeToMutate();
    std::scoped_lock lock(rewindMutex);
    if (rewindBuffer.empty()) {
        return;
    }

    SnapshotDelta delta = std::move(rewindBuffer.back());
    rewindBuffer.pop_back();

    if (delta.isKeyframe) {
        lastCapturedState = std::move(delta.fullState);
    } else {
        for (const auto& chunk : delta.chunks) {
            if (chunk.offset + chunk.xorData.size() <= lastCapturedState.size()) {
                for (size_t j = 0; j < chunk.xorData.size(); ++j) {
                    lastCapturedState.at(chunk.offset + j) ^= static_cast<char>(chunk.xorData.at(j));
                }
            }
        }
    }

    lastCapturedCycles = delta.totalCycles;
    lastCapturedInstructions = delta.totalInstructions;
    totalCycles.store(delta.totalCycles, std::memory_order_relaxed);
    totalInstructions.store(delta.totalInstructions, std::memory_order_relaxed);

    std::stringstream stateStream(lastCapturedState);
    if (LoadComponentsState(stateStream)) {
        LoadInternalState(stateStream);
    }
    halted = false;
    pendingInterruptAny.store(pendingIRQ.load() || pendingNMI.load() || via.isIRQAsserted() || acia.HasIRQ(),
                              std::memory_order_relaxed);
}

bool Emulator::CanRewind() const {
    std::scoped_lock lock(rewindMutex);
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
    if (!SaveComponentsState(stateStream)) {
        return;
    }
    SaveInternalState(stateStream);

    std::scoped_lock lock(rewindMutex);
    std::string currentStateStr = stateStream.str();
    uint64_t currentCycles = totalCycles.load(std::memory_order_relaxed);
    uint64_t currentInstructions = totalInstructions.load(std::memory_order_relaxed);

    if (lastCapturedState.empty()) {
        lastCapturedState = std::move(currentStateStr);
        lastCapturedCycles = currentCycles;
        lastCapturedInstructions = currentInstructions;
        return;
    }

    SnapshotDelta delta =
        CreateXORDiff(lastCapturedState, currentStateStr, lastCapturedCycles, lastCapturedInstructions);
    rewindBuffer.push_back(std::move(delta));
    if (rewindBuffer.size() > MAX_REWIND_STATES) {
        rewindBuffer.pop_front();
    }

    lastCapturedState = std::move(currentStateStr);
    lastCapturedCycles = currentCycles;
    lastCapturedInstructions = currentInstructions;
}

}  // namespace Core
