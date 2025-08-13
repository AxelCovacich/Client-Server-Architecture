#include "logger.h"
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

static FILE *pFile = NULL;

int logger_init(const char *log_path) {
    // Initialization code for logger (e.g., open file at log_path)

    mkdir(LOG_DIR, 0777);

    pFile = fopen(log_path, "a");
    if (pFile == NULL) {
        perror("Failed to open log file");
        return -1;
    }
    return 0;
}

void logger_log(const char *component, log_level level, const char *message) {
    if (pFile == NULL) {
        fprintf(stderr, "Logger not initialized.\n");
        return;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm *utc_time = gmtime(&now);
    char buffer[BUFFER_TIME_SIZE];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", utc_time);

    // Write log entry
    fprintf(pFile, "[%s] [%s] [%s]: %s\n", buffer, component, log_level_to_string(level), message);
    fflush(pFile); // Ensure the log is written immediately
}

void logger_close() {
    if (pFile != NULL) {
        fclose(pFile);
        pFile = NULL;
    }
}

const char *log_level_to_string(log_level level) {
    switch (level) {
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}