#include "logger.hpp"
#include <array>
#include <ctime>
#include <iostream>

Logger::Logger(Storage &storage, const IClock &clock, std::ostream &errorStream)
    : m_storage(storage)
    , m_clock(clock)
    , m_errorStream(errorStream) {
}

void Logger::log(LogLevel level, const std::string &component, const std::string &message,
                 const std::optional<std::string> &clientId) noexcept {

    time_t now = m_clock.now();
    tm *UTCTime = gmtime(&now);
    std::array<char, DATE_BUFFER_SIZE> buffer{}; // Buffer for message
    strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", UTCTime);
    std::string date = std::string(buffer.data()) + " UTC";

    std::string level_str = levelToString(level);

    // std::cout << "[" << date << "] [" << component << "] [" << level_str << "] " << message << '\n';

    try {
        std::string level_str = levelToString(level);
        m_storage.saveLogEntry(date, level_str, component, message, clientId);

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
