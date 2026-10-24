#include "Hardware/Core/SymbolTable.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <system_error>

class SymbolTable_Test : public ::testing::Test {
   protected:
    void SetUp() override {
        testDbgPath = std::filesystem::temp_directory_path() / "test_symbols.dbg";
        std::ofstream ofs(testDbgPath, std::ios::trunc);
        ofs << "sym\tid=0,name=\"reset\",val=0x8000\n";
        ofs << "sym\tid=1,name=\"_main\",val=0x8020\n";
        ofs << "sym\tid=2,name=\"foo\",val=0x8040\n";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(testDbgPath, ec);
    }

    std::filesystem::path testDbgPath;
};

TEST_F(SymbolTable_Test, FindSymbolAndClear) {
    Control::SymbolTable symTable;
    ASSERT_TRUE(symTable.LoadFromFile(testDbgPath.string()));
    EXPECT_TRUE(symTable.IsLoaded());
    EXPECT_EQ(symTable.GetSymbolCount(), 3u);

    EXPECT_EQ(symTable.FindSymbol(""), nullptr);

    const auto* symReset = symTable.FindSymbol("reset");
    ASSERT_NE(symReset, nullptr);
    EXPECT_EQ(symReset->name, "reset");
    EXPECT_EQ(symReset->val, 0x8000);

    const auto* symMainExact = symTable.FindSymbol("_main");
    ASSERT_NE(symMainExact, nullptr);
    EXPECT_EQ(symMainExact->name, "_main");
    EXPECT_EQ(symMainExact->val, 0x8020);

    const auto* symMainPrefixed = symTable.FindSymbol("main");
    ASSERT_NE(symMainPrefixed, nullptr);
    EXPECT_EQ(symMainPrefixed->name, "_main");
    EXPECT_EQ(symMainPrefixed->val, 0x8020);

    const auto* symFooWithoutUnderscore = symTable.FindSymbol("_foo");
    ASSERT_NE(symFooWithoutUnderscore, nullptr);
    EXPECT_EQ(symFooWithoutUnderscore->name, "foo");
    EXPECT_EQ(symFooWithoutUnderscore->val, 0x8040);

    EXPECT_EQ(symTable.FindSymbol("missing"), nullptr);
    EXPECT_EQ(symTable.FindSymbol("_missing"), nullptr);

    symTable.Clear();
    EXPECT_FALSE(symTable.IsLoaded());
    EXPECT_EQ(symTable.GetSymbolCount(), 0u);
    EXPECT_EQ(symTable.FindSymbol("reset"), nullptr);
    EXPECT_EQ(symTable.FindSymbol("main"), nullptr);
    EXPECT_EQ(symTable.FindSymbol("_main"), nullptr);
    EXPECT_EQ(symTable.FindSymbol("foo"), nullptr);
    EXPECT_EQ(symTable.FindSymbol("_foo"), nullptr);
}
