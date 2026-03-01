#include "ConsoleLogger.h"

#include <iostream>

namespace config::logger
{

ConsoleLogger::ConsoleLogger(const LogLevel level) : m_level(level) {}

void ConsoleLogger::log(const LogLevel& level, const std::string_view message) const
{
    if (level >= m_level)
    {
        std::cout << std::format("{}: {}", level, message) << std::endl;
    }
}

}