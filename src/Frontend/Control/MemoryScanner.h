#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

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

namespace Detail {

inline std::size_t ScanSize(const ScanQuery& query) {
    switch (query.type) {
        case ScanType::U8:
            return 1;
        case ScanType::U16:
            return 2;
        default:
            return query.text.size();
    }
}

template <typename ReadFn>
std::uint16_t ReadNumber(ScanType type, std::uint16_t addr, ReadFn& read) {
    auto low = static_cast<std::uint16_t>(read(addr));
    if (type == ScanType::U8) {
        return low;
    }
    return static_cast<std::uint16_t>(low | (read(static_cast<std::uint16_t>(addr + 1)) << 8U));
}

template <typename ReadFn>
bool MatchesText(const std::string& text, std::uint16_t addr, ReadFn& read) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (read(static_cast<std::uint16_t>(addr + i)) != static_cast<std::uint8_t>(text[i])) {
            return false;
        }
    }
    return true;
}

}  // namespace Detail

// read: callable (uint16_t address) -> uint8_t. unknown: snapshot every address, numeric types only.
template <typename ReadFn>
std::vector<ScanCandidate> FirstScan(const ScanQuery& query, std::uint16_t start, std::uint16_t end, bool unknown,
                                     ReadFn read) {
    std::vector<ScanCandidate> result;
    const std::size_t size = Detail::ScanSize(query);
    if (size == 0) {
        return result;
    }
    for (std::size_t addr = start; addr + size - 1 <= end; ++addr) {
        auto address = static_cast<std::uint16_t>(addr);
        if (query.type == ScanType::String) {
            if (Detail::MatchesText(query.text, address, read)) {
                result.push_back({address, 0});
            }
            continue;
        }
        std::uint16_t current = Detail::ReadNumber(query.type, address, read);
        if (unknown || current == query.value) {
            result.push_back({address, current});
        }
    }
    return result;
}

// Filters candidates in place and stores the current value as `previous`.
template <typename ReadFn>
void NextScan(std::vector<ScanCandidate>& candidates, const ScanQuery& query, ReadFn read) {
    auto keep = [&](ScanCandidate& cand) {
        if (query.type == ScanType::String) {
            return Detail::MatchesText(query.text, cand.address, read);
        }
        std::uint16_t current = Detail::ReadNumber(query.type, cand.address, read);
        std::uint16_t before = cand.previous;
        cand.previous = current;
        switch (query.filter) {
            case ScanFilter::Exact:
                return current == query.value;
            case ScanFilter::Increased:
                return current > before;
            case ScanFilter::Decreased:
                return current < before;
            default:
                return current == before;
        }
    };
    std::erase_if(candidates, [&](ScanCandidate& cand) { return !keep(cand); });
}

}  // namespace Control

