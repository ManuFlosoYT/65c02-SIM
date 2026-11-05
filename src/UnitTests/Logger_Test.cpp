#include "Hardware/Core/Logger.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <vector>

namespace {

class TestCaptureSink : public Core::ILogSink {
public:
    struct Entry {
        Core::LogLevel level;
        std::string message;
    };

    void Write(Core::LogLevel level, std::string_view message) override {
        entries.push_back(Entry{level, std::string(message)});
    }

    std::vector<Entry> entries;
};

}  // namespace

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& logger = Core::Logger::Instance();
        logger.SetLevel(Core::LogLevel::Trace);
        logger.ClearSinks();
    }

    void TearDown() override {
        auto& logger = Core::Logger::Instance();
        logger.SetMode(Core::LogMode::Discard);
        logger.SetLevel(Core::LogLevel::Info);
    }
};

TEST_F(LoggerTest, DiscardModeOutputsNothing) {
    auto& logger = Core::Logger::Instance();
    logger.SetMode(Core::LogMode::Discard);

    Core::Logger::Info("Test message in discard mode");
    Core::Logger::Error("Error in discard mode");
    EXPECT_EQ(logger.GetMode(), Core::LogMode::Discard);
}

TEST_F(LoggerTest, CustomSinkCapturesMessages) {
    auto sink = std::make_shared<TestCaptureSink>();
    auto& logger = Core::Logger::Instance();
    logger.AddSink(sink);

    Core::Logger::Info("Information: {}", 42);
    Core::Logger::Warn("Warning alert");
    Core::Logger::Error("Failed code: {}", 0x1234);

    ASSERT_EQ(sink->entries.size(), 3U);
    EXPECT_EQ(sink->entries[0].level, Core::LogLevel::Info);
    EXPECT_EQ(sink->entries[0].message, "Information: 42");

    EXPECT_EQ(sink->entries[1].level, Core::LogLevel::Warn);
    EXPECT_EQ(sink->entries[1].message, "Warning alert");

    EXPECT_EQ(sink->entries[2].level, Core::LogLevel::Error);
    EXPECT_EQ(sink->entries[2].message, "Failed code: 4660");
}

TEST_F(LoggerTest, LevelFilteringWorks) {
    auto sink = std::make_shared<TestCaptureSink>();
    auto& logger = Core::Logger::Instance();
    logger.AddSink(sink);
    logger.SetLevel(Core::LogLevel::Warn);

    Core::Logger::Trace("Ignored trace");
    Core::Logger::Debug("Ignored debug");
    Core::Logger::Info("Ignored info");
    Core::Logger::Warn("Logged warn");
    Core::Logger::Error("Logged error");

    ASSERT_EQ(sink->entries.size(), 2U);
    EXPECT_EQ(sink->entries[0].level, Core::LogLevel::Warn);
    EXPECT_EQ(sink->entries[1].level, Core::LogLevel::Error);
}

TEST_F(LoggerTest, OffLevelSuppressesAll) {
    auto sink = std::make_shared<TestCaptureSink>();
    auto& logger = Core::Logger::Instance();
    logger.AddSink(sink);
    logger.SetLevel(Core::LogLevel::Off);

    Core::Logger::Critical("Ignored critical");
    Core::Logger::Error("Ignored error");

    EXPECT_TRUE(sink->entries.empty());
}

TEST_F(LoggerTest, FileSinkWritesToFile) {
    const std::string tempLogPath = "logger_unit_test.tmp.log";
    if (std::filesystem::exists(tempLogPath)) {
        std::filesystem::remove(tempLogPath);
    }

    auto& logger = Core::Logger::Instance();
    logger.SetFileSink(tempLogPath);

    Core::Logger::Info("Written to file");
    logger.Flush();

    EXPECT_TRUE(std::filesystem::exists(tempLogPath));
    std::ifstream in(tempLogPath);
    std::string line;
    std::getline(in, line);
    in.close();
    std::filesystem::remove(tempLogPath);

    EXPECT_NE(line.find("[INFO] Written to file"), std::string::npos);
}

TEST_F(LoggerTest, MultiThreadedLoggingSafety) {
    auto sink = std::make_shared<TestCaptureSink>();
    auto& logger = Core::Logger::Instance();
    logger.AddSink(sink);

    constexpr int threadCount = 4;
    constexpr int messagesPerThread = 50;
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                Core::Logger::Info("Thread {} message {}", t, i);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(sink->entries.size(), static_cast<size_t>(threadCount * messagesPerThread));
}
