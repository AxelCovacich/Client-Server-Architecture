#include "logger.hpp"
#include <iostream>

Logger::Logger(Storage &storage, const IClock &clock, std::ostream &errorStream)
    : m_storage(storage)
    , m_clock(clock)
    , m_errorStream(errorStream) {
}

void Logger::log(LogLevel level, const std::string &component, const std::string &message,
                 const std::optional<std::string> &clientId) noexcept {

    std::time_t now = m_clock.now();
    std::string level_str = levelToString(level);

    // std::cout << "[" << now << "] [" << component << "] [" << level_str << "] " << message << '\n';

    try {
        std::string level_str = levelToString(level);
        m_storage.saveLogEntry(now, level_str, component, message, clientId);

    } catch (const std::exception &e) {

        m_errorStream << "CRITICAL LOGGER FAILURE: Could not persist log. DB Error: " << e.what() << '\n';
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERROR:
        return "ERROR";
    }

    throw std::logic_error("Invalid or unhandled LogLevel in levelToString");
}
