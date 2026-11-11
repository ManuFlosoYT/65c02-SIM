#include <gtest/gtest.h>

#include "Frontend/Control/UIModalState.h"

TEST(BackendWatchdog_Test, DoesNotTriggerWhenPausedOrInactive) {
    Control::PopupState popups;
    popups.UpdateWatchdog(false, 100, 0);
    EXPECT_FALSE(popups.backendFrozen);

    popups.UpdateWatchdog(false, 100, 6000);
    EXPECT_FALSE(popups.backendFrozen);
}

TEST(BackendWatchdog_Test, DoesNotTriggerWhileCyclesAdvance) {
    Control::PopupState popups;
    popups.UpdateWatchdog(true, 100, 1000);
    EXPECT_FALSE(popups.backendFrozen);

    popups.UpdateWatchdog(true, 100, 5999);
    EXPECT_FALSE(popups.backendFrozen);

    popups.UpdateWatchdog(true, 101, 5999);
    EXPECT_FALSE(popups.backendFrozen);

    popups.UpdateWatchdog(true, 101, 10998);
    EXPECT_FALSE(popups.backendFrozen);
}

TEST(BackendWatchdog_Test, TriggersAfterFiveSecondsWithoutCycleProgress) {
    Control::PopupState popups;
    popups.UpdateWatchdog(true, 500, 1000);
    EXPECT_FALSE(popups.backendFrozen);

    popups.UpdateWatchdog(true, 500, 5999);
    EXPECT_FALSE(popups.backendFrozen);

    popups.UpdateWatchdog(true, 500, 6000);
    EXPECT_TRUE(popups.backendFrozen);
}

TEST(BackendWatchdog_Test, ResetWatchdogClearsStateAndAllowsRetracking) {
    Control::PopupState popups;
    popups.UpdateWatchdog(true, 500, 1000);
    popups.UpdateWatchdog(true, 500, 6000);
    ASSERT_TRUE(popups.backendFrozen);

    popups.ResetWatchdog();
    EXPECT_FALSE(popups.backendFrozen);
    EXPECT_FALSE(popups.watchdogTracking);

    popups.UpdateWatchdog(true, 500, 10000);
    EXPECT_FALSE(popups.backendFrozen);
    EXPECT_TRUE(popups.watchdogTracking);

    popups.UpdateWatchdog(true, 500, 14999);
    EXPECT_FALSE(popups.backendFrozen);

    popups.UpdateWatchdog(true, 500, 15000);
    EXPECT_TRUE(popups.backendFrozen);
}
