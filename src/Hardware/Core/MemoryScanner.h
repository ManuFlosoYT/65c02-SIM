#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Hardware {
class Bus;
}

namespace Control {

enum class ScanType : std::uint8_t { U8, U16, String };
enum class ScanFilter : std::uint8_t { Exact, Increased, Decreased, Unchanged };

struct ScanCandidate {
    std::uint16_t address;
    std::uint16_t previous;  // unused for String
};

struct ScanQuery {
    ScanType type = ScanType::U8;
    ScanFilter filter = ScanFilter::Exact;
    std::uint16_t value = 0;
    std::string text;
};

struct ScannerState {
    ScanQuery query;
    std::uint16_t startAddr = 0x0000;
    std::uint16_t endAddr = 0xFFFF;
    bool unknown = false;
    std::vector<ScanCandidate> candidates;
    bool scanned = false;
    char text[64] = "";

    void Clear() {
        candidates.clear();
        scanned = false;
    }
};

using ReadFn = std::function<uint8_t(uint16_t)>;

std::vector<ScanCandidate> FirstScan(const ScanQuery& query, std::uint16_t start, std::uint16_t end, bool unknown,
                                     const ReadFn& read);
void NextScan(std::vector<ScanCandidate>& candidates, const ScanQuery& query, const ReadFn& read);
void ClearScan(ScannerState& state);
std::string ReadText(const ReadFn& read, std::uint16_t addr, std::size_t length);
std::string ReadText(Hardware::Bus& bus, std::uint16_t addr, std::size_t length);

}  // namespace Control

