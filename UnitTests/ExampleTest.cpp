#include <gtest/gtest.h>

TEST(ExampleTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

TEST(ExampleTest, MIPS_Component_Test) {
    // This is a placeholder for testing MIPS components.
    // Ensure you can include headers from your core library.
    // #include "../Componentes/CPU.h"
    // CPU cpu;
    // EXPECT_TRUE(true);
}
