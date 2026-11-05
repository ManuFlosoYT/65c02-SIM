#include "Hardware/Core/Logger.h"

namespace Core {

void ConsoleSink::Write(LogLevel level, std::string_view message) {
    if (level >= LogLevel::Warn) {
        std::cerr << message << '\n';
    } else {
        std::cout << message << '\n';
    }
}

void ConsoleSink::Flush() {
    std::cout.flush();
    std::cerr.flush();
}

FileSink::FileSink(const std::string& filepath)
    : m_file(filepath, std::ios::out | std::ios::app) {}

void FileSink::Write(LogLevel level, std::string_view message) {
    if (!m_file.is_open()) {
        return;
    }
    const char* levelStr = "";
    switch (level) {
        case LogLevel::Trace: levelStr = "[TRACE] "; break;
        case LogLevel::Debug: levelStr = "[DEBUG] "; break;
        case LogLevel::Info: levelStr = "[INFO] "; break;
        case LogLevel::Warn: levelStr = "[WARN] "; break;
        case LogLevel::Error: levelStr = "[ERROR] "; break;
        case LogLevel::Critical: levelStr = "[CRIT] "; break;
        default: break;
    }
    m_file << levelStr << message << '\n';
}

void FileSink::Flush() {
    if (m_file.is_open()) {
        m_file.flush();
    }
}

Logger::Logger() {
    m_sinks.push_back(std::make_shared<NullSink>());
}

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::SetLevel(LogLevel level) noexcept {
    m_level = level;
}

LogLevel Logger::GetLevel() const noexcept {
    return m_level;
}

void Logger::SetMode(LogMode mode) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mode = mode;
    m_sinks.clear();
    if (mode == LogMode::Discard) {
        m_sinks.push_back(std::make_shared<NullSink>());
    } else if (mode == LogMode::Console) {
        m_sinks.push_back(std::make_shared<ConsoleSink>());
    }
}

LogMode Logger::GetMode() const noexcept {
    return m_mode;
}

void Logger::AddSink(std::shared_ptr<ILogSink> sink) {
    if (!sink) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sinks.push_back(std::move(sink));
    m_mode = LogMode::Custom;
}

void Logger::ClearSinks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sinks.clear();
    m_mode = LogMode::Custom;
}

void Logger::SetFileSink(const std::string& path) {
    AddSink(std::make_shared<FileSink>(path));
}

bool Logger::ShouldLog(LogLevel level) const noexcept {
    return level >= m_level && m_level != LogLevel::Off;
}

void Logger::Log(LogLevel level, std::string_view message) {
    if (!ShouldLog(level)) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sink : m_sinks) {
        sink->Write(level, message);
    }
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sink : m_sinks) {
        sink->Flush();
    }
}

}  // namespace Core
