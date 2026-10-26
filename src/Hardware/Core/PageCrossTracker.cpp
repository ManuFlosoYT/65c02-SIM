#include "Hardware/Core/PageCrossTracker.h"

namespace Hardware {

void PageCrossTracker::OnPageCross(uint16_t pc, uint16_t baseAddr, uint16_t effectiveAddr,
                                   uint16_t subroutineAddr, const std::string& subroutineName) {
    for (auto& alert : alerts) {
        if (alert.pc == pc && alert.baseAddr == baseAddr && alert.effectiveAddr == effectiveAddr) {
            alert.count++;
            return;
        }
    }

    PageCrossAlert alert;
    alert.pc = pc;
    alert.baseAddr = baseAddr;
    alert.effectiveAddr = effectiveAddr;
    alert.subroutineAddr = subroutineAddr;
    alert.subroutineName = subroutineName;
    alert.count = 1;
    alerts.push_back(std::move(alert));
}

void PageCrossTracker::ClearAlerts() {
    alerts.clear();
}

uint32_t PageCrossTracker::GetTotalPenalties() const {
    uint32_t total = 0;
    for (const auto& alert : alerts) {
        total += alert.count;
    }
    return total;
}

}  // namespace Hardware
