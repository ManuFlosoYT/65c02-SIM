#include "Hardware/Core/SubroutineProfiler.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <system_error>

#include "Hardware/Core/SymbolTable.h"

TEST(SubroutineProfilerTest, DisabledByDefault) {
    Hardware::SubroutineProfiler profiler;
    EXPECT_FALSE(profiler.IsEnabled());

    profiler.OnJSR(0x1000, 0x0200, 100);
    profiler.OnPageCross(0x1005, 0x20F0, 0x2105);
    profiler.OnRTS(200);

    auto snapshot = profiler.GetSnapshot();
    EXPECT_EQ(snapshot.totalCycles, 0);
    EXPECT_TRUE(snapshot.root->children.empty());
    EXPECT_TRUE(snapshot.pageCrossAlerts.empty());

    profiler.SetEnabled(true);
    EXPECT_TRUE(profiler.IsEnabled());
    profiler.SetEnabled(false);
    EXPECT_FALSE(profiler.IsEnabled());
}

TEST(SubroutineProfilerTest, JSRAndRTSCycleCalculationAndChildNesting) {
    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    profiler.OnJSR(0x1000, 0x0200, 100);
    profiler.OnJSR(0x2000, 0x1010, 120);
    profiler.OnRTS(170);
    profiler.OnRTS(200);

    auto snapshot = profiler.GetSnapshot();
    EXPECT_EQ(snapshot.totalCycles, 100);
    ASSERT_EQ(snapshot.root->children.size(), 1u);

    const auto& node1000 = snapshot.root->children[0];
    EXPECT_EQ(node1000->address, 0x1000);
    EXPECT_EQ(node1000->name, "$1000");
    EXPECT_EQ(node1000->totalCycles, 100);
    EXPECT_EQ(node1000->selfCycles, 50);
    EXPECT_EQ(node1000->callCount, 1u);
    EXPECT_FALSE(node1000->isInterrupt);
    ASSERT_EQ(node1000->children.size(), 1u);

    const auto& node2000 = node1000->children[0];
    EXPECT_EQ(node2000->address, 0x2000);
    EXPECT_EQ(node2000->name, "$2000");
    EXPECT_EQ(node2000->totalCycles, 50);
    EXPECT_EQ(node2000->selfCycles, 50);
    EXPECT_EQ(node2000->callCount, 1u);
    EXPECT_FALSE(node2000->isInterrupt);
    EXPECT_TRUE(node2000->children.empty());
}

TEST(SubroutineProfilerTest, MergedAndMultipleCalls) {
    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    profiler.OnJSR(0x1000, 0x0200, 100);
    profiler.OnJSR(0x2000, 0x1010, 120);
    profiler.OnRTS(150);
    profiler.OnJSR(0x2000, 0x1020, 160);
    profiler.OnRTS(180);
    profiler.OnJSR(0x3000, 0x1030, 190);
    profiler.OnRTS(220);
    profiler.OnRTS(250);

    auto snapshot = profiler.GetSnapshot();
    EXPECT_EQ(snapshot.totalCycles, 150);
    ASSERT_EQ(snapshot.root->children.size(), 1u);

    const auto& node1000 = snapshot.root->children[0];
    EXPECT_EQ(node1000->totalCycles, 150);
    EXPECT_EQ(node1000->selfCycles, 70);
    EXPECT_EQ(node1000->callCount, 1u);
    ASSERT_EQ(node1000->children.size(), 2u);

    const auto& node2000 = node1000->children[0];
    EXPECT_EQ(node2000->address, 0x2000);
    EXPECT_EQ(node2000->callCount, 2u);
    EXPECT_EQ(node2000->totalCycles, 50);
    EXPECT_EQ(node2000->selfCycles, 50);

    const auto& node3000 = node1000->children[1];
    EXPECT_EQ(node3000->address, 0x3000);
    EXPECT_EQ(node3000->callCount, 1u);
    EXPECT_EQ(node3000->totalCycles, 30);
    EXPECT_EQ(node3000->selfCycles, 30);
}

TEST(SubroutineProfilerTest, BRKAndRTIInterruptProfiling) {
    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    profiler.OnBRK(0xFFFE, 0x1234, 100);
    profiler.OnRTI(160);

    auto snapshot = profiler.GetSnapshot();
    EXPECT_EQ(snapshot.totalCycles, 60);
    ASSERT_EQ(snapshot.root->children.size(), 1u);

    const auto& brkNode = snapshot.root->children[0];
    EXPECT_EQ(brkNode->address, 0xFFFE);
    EXPECT_TRUE(brkNode->isInterrupt);
    EXPECT_EQ(brkNode->callCount, 1u);
    EXPECT_EQ(brkNode->totalCycles, 60);
    EXPECT_EQ(brkNode->selfCycles, 60);
}

TEST(SubroutineProfilerTest, NestedInterruptWithinSubroutine) {
    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    profiler.OnJSR(0x1000, 0x0200, 100);
    profiler.OnInterrupt(0xFFFA, 130, "NMI_Handler");
    profiler.OnRTI(170);
    profiler.OnRTS(200);

    auto snapshot = profiler.GetSnapshot();
    EXPECT_EQ(snapshot.totalCycles, 100);
    ASSERT_EQ(snapshot.root->children.size(), 1u);

    const auto& sub = snapshot.root->children[0];
    EXPECT_EQ(sub->totalCycles, 100);
    EXPECT_EQ(sub->selfCycles, 60);
    ASSERT_EQ(sub->children.size(), 1u);

    const auto& nmi = sub->children[0];
    EXPECT_EQ(nmi->address, 0xFFFA);
    EXPECT_EQ(nmi->name, "NMI_Handler");
    EXPECT_TRUE(nmi->isInterrupt);
    EXPECT_EQ(nmi->totalCycles, 40);
    EXPECT_EQ(nmi->selfCycles, 40);
    EXPECT_EQ(nmi->callCount, 1u);
}

TEST(SubroutineProfilerTest, ActiveStackCyclesInSnapshot) {
    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    profiler.OnJSR(0x1000, 0x0200, 100);
    profiler.OnJSR(0x2000, 0x1010, 150);
    profiler.OnInterrupt(0xFFFE, 200, "IRQ");

    auto snapshot = profiler.GetSnapshot();
    EXPECT_EQ(snapshot.totalCycles, 100);
    ASSERT_EQ(snapshot.root->children.size(), 1u);

    const auto& sub1000 = snapshot.root->children[0];
    EXPECT_EQ(sub1000->totalCycles, 100);
    EXPECT_EQ(sub1000->selfCycles, 50);
    ASSERT_EQ(sub1000->children.size(), 1u);

    const auto& sub2000 = sub1000->children[0];
    EXPECT_EQ(sub2000->totalCycles, 50);
    EXPECT_EQ(sub2000->selfCycles, 50);
    ASSERT_EQ(sub2000->children.size(), 1u);

    const auto& irq = sub2000->children[0];
    EXPECT_EQ(irq->totalCycles, 0);
    EXPECT_EQ(irq->selfCycles, 0);

    profiler.OnRTI(220);
    profiler.OnRTS(240);
    profiler.OnRTS(250);

    auto finalSnapshot = profiler.GetSnapshot();
    EXPECT_EQ(finalSnapshot.totalCycles, 150);
    ASSERT_EQ(finalSnapshot.root->children.size(), 1u);
    EXPECT_EQ(finalSnapshot.root->children[0]->totalCycles, 150);
    EXPECT_EQ(finalSnapshot.root->children[0]->selfCycles, 60);

    const auto& final2000 = finalSnapshot.root->children[0]->children[0];
    EXPECT_EQ(final2000->totalCycles, 90);
    EXPECT_EQ(final2000->selfCycles, 70);

    const auto& finalIrq = final2000->children[0];
    EXPECT_EQ(finalIrq->totalCycles, 20);
    EXPECT_EQ(finalIrq->selfCycles, 20);
}

TEST(SubroutineProfilerTest, EmptyStackGracefulHandling) {
    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    EXPECT_NO_THROW(profiler.OnRTS(100));
    EXPECT_NO_THROW(profiler.OnRTI(100));

    auto snapshot = profiler.GetSnapshot();
    EXPECT_EQ(snapshot.totalCycles, 0);
    EXPECT_TRUE(snapshot.root->children.empty());
}

TEST(SubroutineProfilerTest, PageCrossAlertRecording) {
    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    profiler.OnJSR(0x1000, 0x0200, 100);
    profiler.OnPageCross(0x1005, 0x20F0, 0x2105);
    profiler.OnPageCross(0x1005, 0x20F0, 0x2105);
    profiler.OnPageCross(0x1020, 0x30F0, 0x3102);
    profiler.OnRTS(200);

    profiler.OnPageCross(0x8000, 0x90F0, 0x9100);

    auto snapshot = profiler.GetSnapshot();
    ASSERT_EQ(snapshot.root->children.size(), 1u);
    EXPECT_EQ(snapshot.root->children[0]->pageCrossPenalties, 3u);
    EXPECT_EQ(snapshot.root->pageCrossPenalties, 1u);

    ASSERT_EQ(snapshot.pageCrossAlerts.size(), 3u);

    EXPECT_EQ(snapshot.pageCrossAlerts[0].pc, 0x1005);
    EXPECT_EQ(snapshot.pageCrossAlerts[0].baseAddr, 0x20F0);
    EXPECT_EQ(snapshot.pageCrossAlerts[0].effectiveAddr, 0x2105);
    EXPECT_EQ(snapshot.pageCrossAlerts[0].subroutineAddr, 0x1000);
    EXPECT_EQ(snapshot.pageCrossAlerts[0].subroutineName, "$1000");
    EXPECT_EQ(snapshot.pageCrossAlerts[0].count, 2u);

    EXPECT_EQ(snapshot.pageCrossAlerts[1].pc, 0x1020);
    EXPECT_EQ(snapshot.pageCrossAlerts[1].baseAddr, 0x30F0);
    EXPECT_EQ(snapshot.pageCrossAlerts[1].effectiveAddr, 0x3102);
    EXPECT_EQ(snapshot.pageCrossAlerts[1].subroutineAddr, 0x1000);
    EXPECT_EQ(snapshot.pageCrossAlerts[1].subroutineName, "$1000");
    EXPECT_EQ(snapshot.pageCrossAlerts[1].count, 1u);

    EXPECT_EQ(snapshot.pageCrossAlerts[2].pc, 0x8000);
    EXPECT_EQ(snapshot.pageCrossAlerts[2].baseAddr, 0x90F0);
    EXPECT_EQ(snapshot.pageCrossAlerts[2].effectiveAddr, 0x9100);
    EXPECT_EQ(snapshot.pageCrossAlerts[2].subroutineAddr, 0x0000);
    EXPECT_EQ(snapshot.pageCrossAlerts[2].subroutineName, "Root");
    EXPECT_EQ(snapshot.pageCrossAlerts[2].count, 1u);
}

TEST(SubroutineProfilerTest, ClearAndReset) {
    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    profiler.OnJSR(0x1000, 0x0200, 100);
    profiler.OnPageCross(0x1005, 0x20F0, 0x2105);
    profiler.OnRTS(200);

    auto snapshot = profiler.GetSnapshot();
    EXPECT_EQ(snapshot.totalCycles, 100);
    EXPECT_FALSE(snapshot.root->children.empty());
    EXPECT_FALSE(snapshot.pageCrossAlerts.empty());

    profiler.Clear();
    auto clearedSnapshot = profiler.GetSnapshot();
    EXPECT_EQ(clearedSnapshot.totalCycles, 0);
    EXPECT_TRUE(clearedSnapshot.root->children.empty());
    EXPECT_TRUE(clearedSnapshot.pageCrossAlerts.empty());

    profiler.OnJSR(0x2000, 0x0200, 300);
    profiler.OnRTS(400);
    EXPECT_FALSE(profiler.GetSnapshot().root->children.empty());

    profiler.Reset();
    auto resetSnapshot = profiler.GetSnapshot();
    EXPECT_EQ(resetSnapshot.totalCycles, 0);
    EXPECT_TRUE(resetSnapshot.root->children.empty());
}

TEST(SubroutineProfilerTest, SymbolResolution) {
    auto dbgPath = std::filesystem::temp_directory_path() / "profiler_test_symbols.dbg";
    {
        std::ofstream ofs(dbgPath, std::ios::trunc);
        ofs << "sym\tid=0,name=\"RenderScreen\",val=0x1000\n";
        ofs << "sym\tid=1,name=\"ComputeAudio\",val=0x2000\n";
    }

    Control::SymbolTable symTable;
    ASSERT_TRUE(symTable.LoadFromFile(dbgPath.string()));

    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);
    profiler.SetSymbolTable(&symTable);

    profiler.OnJSR(0x1000, 0x0200, 100);
    profiler.OnJSR(0x2000, 0x1010, 120);
    profiler.OnRTS(150);
    profiler.OnRTS(200);

    auto snapshot = profiler.GetSnapshot();
    ASSERT_EQ(snapshot.root->children.size(), 1u);
    EXPECT_EQ(snapshot.root->children[0]->name, "RenderScreen");
    ASSERT_EQ(snapshot.root->children[0]->children.size(), 1u);
    EXPECT_EQ(snapshot.root->children[0]->children[0]->name, "ComputeAudio");

    std::error_code ec;
    std::filesystem::remove(dbgPath, ec);
}

TEST(SubroutineProfilerTest, RetroactiveSymbolResolution) {
    auto dbgPath = std::filesystem::temp_directory_path() / "profiler_retro_symbols.dbg";
    {
        std::ofstream ofs(dbgPath, std::ios::trunc);
        ofs << "sym\tid=0,name=\"RetroMain\",val=0x4000\n";
        ofs << "sym\tid=1,name=\"RetroDraw\",val=0x5000\n";
    }

    Hardware::SubroutineProfiler profiler;
    profiler.SetEnabled(true);

    profiler.OnJSR(0x4000, 0x0200, 100);
    profiler.OnJSR(0x5000, 0x4010, 120);
    profiler.OnPageCross(0x5020, 0x20F0, 0x2100);
    profiler.OnRTS(180);
    profiler.OnRTS(250);

    auto snapshotBefore = profiler.GetSnapshot();
    ASSERT_EQ(snapshotBefore.root->children.size(), 1u);
    EXPECT_EQ(snapshotBefore.root->children[0]->name, "$4000");
    ASSERT_EQ(snapshotBefore.pageCrossAlerts.size(), 1u);
    EXPECT_EQ(snapshotBefore.pageCrossAlerts[0].subroutineName, "$5000");

    Control::SymbolTable symTable;
    ASSERT_TRUE(symTable.LoadFromFile(dbgPath.string()));
    profiler.SetSymbolTable(&symTable);

    auto snapshotAfter = profiler.GetSnapshot();
    ASSERT_EQ(snapshotAfter.root->children.size(), 1u);
    EXPECT_EQ(snapshotAfter.root->children[0]->name, "RetroMain");
    ASSERT_EQ(snapshotAfter.root->children[0]->children.size(), 1u);
    EXPECT_EQ(snapshotAfter.root->children[0]->children[0]->name, "RetroDraw");
    ASSERT_EQ(snapshotAfter.pageCrossAlerts.size(), 1u);
    EXPECT_EQ(snapshotAfter.pageCrossAlerts[0].subroutineName, "RetroDraw");

    std::error_code ec;
    std::filesystem::remove(dbgPath, ec);
}
