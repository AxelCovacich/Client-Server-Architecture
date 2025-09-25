#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "clock.hpp"
#include "config.hpp"
#include "storage.hpp"
#include <fstream>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>

#define DATE_BUFFER_SIZE 32
#define BUFFER_CHUNK_RW_SIZE 16384 // 16KB buffer size for file I/O
#define MB_MULTIPLIER 1048576      // 1024 * 1024

enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger {
  public:
    Logger(Storage &storage, const IClock &clock, std::ostream &errorStream, const Config &config) noexcept;

    /**
     * @brief Logs a message to the console and persists it to the database.
     *
     * This function is the primary interface for all system logging. If a database error occurs during
     * persistence, the error will be logged to stderr, but the exception will not
     * be propagated, ensuring the application's stability.
     *
     * @param level The severity level of the log (e.g., LogLevel::INFO).
     * @param component The name of the module or component originating the log.
     * @param message The descriptive log message.
     * @param clientId (Optional) The unique identifier of the client associated with
     * the event. If not provided, the log is treated as a system-level event.
     */
    void log(LogLevel level, const std::string &component, const std::string &message,
             const std::optional<std::string> &clientId = std::nullopt) noexcept;

    /**
     * @brief Converts a LogLevel enum to its string representation.
     *
     * This is a static helper function used for formatting log messages.
     * @param level The LogLevel enum value to convert.
     * @return A string representing the log level (e.g., "INFO", "ERROR").
     * @throw std::logic_error if an unhandled LogLevel value is provided.
     */
    static std::string levelToString(LogLevel level);

    /**
     * @brief Opens the log file for writing.
     * @param filePath The path to the log file.
     * @return True if the file was opened successfully, false otherwise.
     */
    bool openLogFile(const std::string &filePath) noexcept;

    /**
     * @brief Closes the log file if it is open.
     */
    void closeLogFile() noexcept;

    /**
     * @brief Performs log rotation if the current log file exceeds the maximum size.
     *
     * This function checks the size of the current log file against the configured
     * maximum size. If the file exceeds this size, it is compressed and archived,
     * and a new log file is started. Any errors during this process are logged to stderr.
     */
    void logRotation();

    /**
     * @brief Compresses a file using gzip compression.
     * @param srcPath The path to the source file to compress.
     * @param destPath The path where the compressed file will be saved.
     * @return True if compression was successful, false otherwise.
     */
    static bool compressFileGzip(const std::string &srcPath, const std::string &destPath);

    /**
     * @brief Checks if the log file should be rotated based on its size.
     * @return True if the log file exceeds the maximum size and should be rotated, false otherwise.
     */
    bool shouldRotate() const;

    /**
     * @brief Checks if file logging is currently enabled.
     * @return True if file logging is enabled, false otherwise.
     */
    bool isFileEnabled() const;

    /**
     * @brief Checks if the log file is currently open.
     * @return True if the log file is open, false otherwise.
     */
    bool isLogFileOpen() const;

  private:
    std::ostream &m_errorStream;
    Storage &m_storage;
    const IClock &m_clock;
    const Config &m_config;

    // File logging members
    std::ofstream m_logFile;
    std::mutex m_fileMutex;
    bool m_fileEnabled;
};

#endif // LOGGER_HPP