#include "logger.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Internal logger state, only used inside logger.c
static FILE *pFile = NULL;

int logger_init(const char *log_path) {
    // Initialization code for logger (e.g., open file at log_path)

    mkdir(LOG_DIR, S_IRWXU | S_IRWXG | S_IRWXO); // Permits 0777

    pFile = fopen(log_path, "a");
    if (pFile == NULL) {
        perror("Failed to open log file");
        return -1;
    }
    return 0;
}

void logger_log(const char *component, log_level level, const char *message) {
    pthread_mutex_lock(&log_mutex); // Ensure thread safety
    if (pFile == NULL) {
        fprintf(stderr, "Logger not initialized.\n"); // NOLINT
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    // Get current time
    time_t now = time(NULL);
    struct tm *utc_time = gmtime(&now);
    char buffer[BUFFER_TIME_SIZE];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", utc_time);

    // Write log entry
    fprintf(pFile, "[%s] [%s] [%s]: %s\n", buffer, component, log_level_to_string(level), message); // NOLINT
    fflush(pFile);                    // Ensure the log is written immediately
    pthread_mutex_unlock(&log_mutex); // Release the mutex
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
