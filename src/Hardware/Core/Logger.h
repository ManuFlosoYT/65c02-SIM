#pragma once

#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Core {

enum class LogLevel : uint8_t {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

enum class LogMode : uint8_t {
    Discard,
    Console,
    Custom
};

class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void Write(LogLevel level, std::string_view message) = 0;
    virtual void Flush() {}
};

class NullSink final : public ILogSink {
public:
    void Write(LogLevel, std::string_view) override {}
};

class ConsoleSink final : public ILogSink {
public:
    void Write(LogLevel level, std::string_view message) override;
    void Flush() override;
};

class FileSink final : public ILogSink {
public:
    explicit FileSink(const std::string& filepath);
    void Write(LogLevel level, std::string_view message) override;
    void Flush() override;

private:
    std::ofstream m_file;
};

class Logger {
public:
    static Logger& Instance();

    void SetLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel GetLevel() const noexcept;

    void SetMode(LogMode mode);
    [[nodiscard]] LogMode GetMode() const noexcept;

    void AddSink(std::shared_ptr<ILogSink> sink);
    void ClearSinks();
    void SetFileSink(const std::string& path);

    [[nodiscard]] bool ShouldLog(LogLevel level) const noexcept;
    void Log(LogLevel level, std::string_view message);
    void Flush();

    template <typename... Args>
    void LogFormat(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (!ShouldLog(level)) {
            return;
        }
        Log(level, std::format(fmt, std::forward<Args>(args)...));
    }

    static void Trace(std::string_view message) {
        Instance().Log(LogLevel::Trace, message);
    }

    template <typename Arg1, typename... Args>
    static void Trace(std::format_string<Arg1, Args...> fmt, Arg1&& arg1, Args&&... args) {
        Instance().LogFormat(LogLevel::Trace, fmt, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    static void Debug(std::string_view message) {
        Instance().Log(LogLevel::Debug, message);
    }

    template <typename Arg1, typename... Args>
    static void Debug(std::format_string<Arg1, Args...> fmt, Arg1&& arg1, Args&&... args) {
        Instance().LogFormat(LogLevel::Debug, fmt, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    static void Info(std::string_view message) {
        Instance().Log(LogLevel::Info, message);
    }

    template <typename Arg1, typename... Args>
    static void Info(std::format_string<Arg1, Args...> fmt, Arg1&& arg1, Args&&... args) {
        Instance().LogFormat(LogLevel::Info, fmt, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    static void Warn(std::string_view message) {
        Instance().Log(LogLevel::Warn, message);
    }

    template <typename Arg1, typename... Args>
    static void Warn(std::format_string<Arg1, Args...> fmt, Arg1&& arg1, Args&&... args) {
        Instance().LogFormat(LogLevel::Warn, fmt, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    static void Error(std::string_view message) {
        Instance().Log(LogLevel::Error, message);
    }

    template <typename Arg1, typename... Args>
    static void Error(std::format_string<Arg1, Args...> fmt, Arg1&& arg1, Args&&... args) {
        Instance().LogFormat(LogLevel::Error, fmt, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    static void Critical(std::string_view message) {
        Instance().Log(LogLevel::Critical, message);
    }

    template <typename Arg1, typename... Args>
    static void Critical(std::format_string<Arg1, Args...> fmt, Arg1&& arg1, Args&&... args) {
        Instance().LogFormat(LogLevel::Critical, fmt, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    mutable std::mutex m_mutex;
    LogLevel m_level{LogLevel::Info};
    LogMode m_mode{LogMode::Discard};
    std::vector<std::shared_ptr<ILogSink>> m_sinks;
};

}  // namespace Core
