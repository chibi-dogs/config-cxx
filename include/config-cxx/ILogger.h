#pragma once
#include <format>
#include <string_view>

namespace config::logger
{
enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

class ILogger
{
    public:
    virtual ~ILogger() = default;
    virtual void log(const LogLevel& level, std::string_view message) const = 0;
    void debug(std::string_view message) {
        log(LogLevel::Debug, message);
    }

    void info(std::string_view message) {
        log(LogLevel::Info, message);
    }

    void warning(std::string_view message) {
        log(LogLevel::Warning, message);
    }

    void error(std::string_view message) {
        log(LogLevel::Error, message);
    }
};

}
template<>
struct std::formatter<config::logger::LogLevel> : std::formatter<std::string_view> {
    auto format(config::logger::LogLevel level, std::format_context& ctx) const {
        std::string_view name;
        switch (level) {
        case config::logger::LogLevel::Debug:   name = "DEBUG"; break;
        case config::logger::LogLevel::Info:    name = "INFO"; break;
        case config::logger::LogLevel::Warning: name = "WARNING"; break;
        case config::logger::LogLevel::Error:   name = "ERROR"; break;
        }
        return std::formatter<std::string_view>::format(name, ctx);
    }
};
