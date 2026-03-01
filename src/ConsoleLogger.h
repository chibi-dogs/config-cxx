#ifndef CONFIG_CXX_CONSOLELOGGER_H
#define CONFIG_CXX_CONSOLELOGGER_H
#include <config-cxx/ILogger.h>

namespace config::logger
{
class ConsoleLogger final: public ILogger
{
public:
    ~ConsoleLogger() override = default;
    explicit ConsoleLogger(const LogLevel level = LogLevel::Info);
    void log(const LogLevel& level, const std::string_view message) const override;


private:
    LogLevel m_level;

};
}
#endif // CONFIG_CXX_CONSOLELOGGER_H
