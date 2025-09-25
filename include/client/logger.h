#ifndef LOGGER_H
#define LOGGER_H

#define LOG_DIR "./var/logs"             // Directory for logs
#define LOG_PATH "./var/logs/client.log" // Default log path
#define BUFFER_TIME_SIZE 32              // Size for time buffer
#define MAX_LOG_MESSAGE_SIZE 512         // Max size for a log message

typedef enum { DEBUG, INFO, WARNING, ERROR } log_level;

/**
 * @brief Initializes the logger by opening the log file.
 * @param log_path The path to the log file.
 * @return 0 on success, -1 on failure.
 */
int logger_init(const char *log_path);

const char *log_level_to_string(log_level level);

/**
 * @brief Logs a message with a given component and log level.
 * @param component The component name (e.g., "CLIENT", "IPC").
 * @param level The log level (DEBUG, INFO, WARNING, ERROR).
 * @param message The message to log.
 */
void logger_log(const char *component, log_level level, const char *message);

/**
 * @brief Closes the logger and releases resources.
 */
void logger_close(void);

#endif // LOGGER_H