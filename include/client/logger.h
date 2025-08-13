#ifndef LOGGER_H
#define LOGGER_H

#define LOG_DIR "./var/logs"             // Directory for logs
#define LOG_PATH "./var/logs/client.log" // Default log path
#define BUFFER_TIME_SIZE 32              // Size for time buffer

typedef enum { DEBUG, INFO, WARNING, ERROR } log_level;

int logger_init(const char *log_path);

const char *log_level_to_string(log_level level);

void logger_log(const char *component, log_level level, const char *message);

void logger_close(void);

#endif // LOGGER_H