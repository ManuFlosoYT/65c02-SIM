#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Hardware {

struct PageCrossAlert {
    uint16_t pc = 0;
    uint16_t baseAddr = 0;
    uint16_t effectiveAddr = 0;
    uint16_t subroutineAddr = 0;
    std::string subroutineName;
    uint32_t count = 0;
};

class PageCrossTracker {
   public:
    PageCrossTracker() = default;
    ~PageCrossTracker() = default;

    PageCrossTracker(const PageCrossTracker&) = default;
    PageCrossTracker& operator=(const PageCrossTracker&) = default;
    PageCrossTracker(PageCrossTracker&&) noexcept = default;
    PageCrossTracker& operator=(PageCrossTracker&&) noexcept = default;

    void OnPageCross(uint16_t pc, uint16_t baseAddr, uint16_t effectiveAddr,
                     uint16_t subroutineAddr, const std::string& subroutineName);

    void ClearAlerts();

    [[nodiscard]] const std::vector<PageCrossAlert>& GetAlerts() const { return alerts; }
    [[nodiscard]] std::vector<PageCrossAlert>& GetAlerts() { return alerts; }

    [[nodiscard]] uint32_t GetTotalPenalties() const;

   private:
    std::vector<PageCrossAlert> alerts;
};

}  // namespace Hardware
