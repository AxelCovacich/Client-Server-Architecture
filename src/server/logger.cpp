#include "logger.hpp"
#include <array>
#include <ctime>
#include <filesystem>
#include <iostream>

Logger::Logger(Storage &storage, const IClock &clock, std::ostream &errorStream) noexcept
    : m_storage(storage)
    , m_clock(clock)
    , m_errorStream(errorStream)
    , m_fileEnabled(false) {
}

bool Logger::openLogFile(const std::string &filePath) noexcept {
    try {
        std::filesystem::path path(filePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
            // Set permissions to 0755 (rwxr-xr-x) for the directory

            std::filesystem::permissions(path.parent_path(),
                                         std::filesystem::perms::owner_all | std::filesystem::perms::group_read |
                                             std::filesystem::perms::group_exec | std::filesystem::perms::others_read |
                                             std::filesystem::perms::others_exec,
                                         std::filesystem::perm_options::replace);
        }
        // open in append mode so existing logs are preserved
        m_logFile.open(filePath, std::ios::out | std::ios::app);
        if (!m_logFile.is_open()) {
            m_errorStream << "Logger: failed to open log file: " << filePath << '\n';
            m_fileEnabled = false;
            return false;
        }
        m_fileEnabled = true;
        return true;
    } catch (const std::exception &e) {
        m_errorStream << "Logger: exception opening log file: " << e.what() << '\n';
        m_fileEnabled = false;
        return false;
    }
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

    // Also write to file if enabled (best-effort; never throw)
    if (m_fileEnabled) {
        try {
            std::lock_guard<std::mutex> lock(m_fileMutex);
            m_logFile << "[" << date << "]" << " [" << component << "]" << " [" << level_str << "] " << message;
            if (clientId) {
                m_logFile << " (client: " << *clientId << ")";
            }
            m_logFile << '\n';
            m_logFile.flush(); // ensure durability; for perf you may buffer and flush periodically
        } catch (const std::exception &e) {
            m_errorStream << "Logger: failed to write to log file: " << e.what() << '\n';
            // disable to avoid repeated errors
            m_fileEnabled = false;
        }
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
