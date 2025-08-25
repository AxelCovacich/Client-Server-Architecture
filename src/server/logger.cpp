#include "logger.hpp"
#include <array>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <zlib.h>

Logger::Logger(Storage &storage, const IClock &clock, std::ostream &errorStream, const Config &config) noexcept
    : m_storage(storage)
    , m_clock(clock)
    , m_errorStream(errorStream)
    , m_fileEnabled(false)
    , m_config(config) {
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

    if (shouldRotate()) {
        logRotation();
    }

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

void Logger::closeLogFile() noexcept {
    std::lock_guard<std::mutex> lock(m_fileMutex);
    if (m_logFile.is_open()) {
        m_logFile.close();
        m_fileEnabled = false;
    }
}

void Logger::logRotation() {
    std::lock_guard<std::mutex> lock(m_fileMutex);

    if (m_logFile.is_open()) { // Close current log file if open
        m_logFile.close();
        m_fileEnabled = false;
    }

    // Rotate log file by renaming it with a timestamp
    std::string oldLogPath = m_config.getLogPath();
    std::filesystem::path oldPath(oldLogPath);
    if (std::filesystem::exists(oldPath)) {
        std::string newLogPath = oldPath.parent_path().string() + "/server_" + std::to_string(m_clock.now()) +
            ".log"; // e.g., server_1633036800.log
        std::filesystem::rename(oldPath, newLogPath);

        // Compress the rotated log
        std::string compressedPath = newLogPath + ".gz"; // e.g., server_1633036800.log.gz
        if (compressFileGzip(newLogPath, compressedPath)) {
            std::filesystem::remove(newLogPath); // Remove uncompressed log after compression
        } else {
            m_errorStream << "Logger: Failed to compress rotated log file.\n";
        }
    }

    if (openLogFile(m_config.getLogPath())) { // Reopen the log file (a new file will be created)
        std::cout << "Logger: Log rotation completed successfully.\n";
    } else {
        m_errorStream << "Logger: Failed to reopen log file after rotation.\n";
    }
}

bool Logger::compressFileGzip(const std::string &srcPath, const std::string &destPath) {

    constexpr size_t BUFSIZE =
        BUFFER_CHUNK_RW_SIZE; // 16 KB buffer size for gzip compression. Will be reading and writing in 16KB chunks
    std::array<char, BUFSIZE> readBuffer{};

    FILE *src = fopen(srcPath.c_str(), "rb"); // NOLINT
    if (src == nullptr) {
        return false;
    }
    gzFile dest = gzopen(destPath.c_str(), "wb");
    if (dest == nullptr) {
        fclose(src); // NOLINT
        return false;
    }

    size_t bytes = 0;
    while ((bytes = fread(readBuffer.data(), 1, BUFSIZE, src)) > 0) {
        if (gzwrite(dest, readBuffer.data(), bytes) != (int)bytes) {
            gzclose(dest);
            fclose(src); // NOLINT
            return false;
        }
    }

    gzclose(dest);
    fclose(src); // NOLINT
    return true;
}

bool Logger::shouldRotate() const {

    try {
        auto logSize = std::filesystem::file_size(m_config.getLogPath());
        uintmax_t thresholdSize =
            static_cast<uintmax_t>(m_config.getMaxLogSize()) * MB_MULTIPLIER; // Convert MB to bytes
        return logSize >= thresholdSize;
    } catch (const std::exception &e) {
        m_errorStream << "Logger: Failed to check log file size: " << e.what() << '\n';
        return false;
    }
}

bool Logger::isFileEnabled() const {
    return m_fileEnabled;
}

bool Logger::isLogFileOpen() const {
    return m_logFile.is_open();
}