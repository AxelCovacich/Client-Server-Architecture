#include "logger.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Internal logger state, only used inside logger.c
static FILE *pFile = NULL;

int logger_init(const char *log_path) {
    // Initialization code for logger (e.g., open file at log_path)

    mkdir(LOG_DIR, S_IRWXU | S_IXGRP | S_IXOTH | S_IRGRP | S_IROTH); // 0755 (rwxr-xr-x).
    pFile = fopen(log_path, "a");
    if (pFile == NULL) {
        perror("Failed to open log file");
        return -1;
    }
    return 0;
}

void logger_log(const char *component, log_level level, const char *format, ...) {
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

    // Print log prefix
    fprintf(pFile, "[%s] [%s] [%s]: ", buffer, component, log_level_to_string(level)); // NOLINT
    // Print formatted message
    va_list args;
    va_start(args, format);
    vfprintf(pFile, format, args);
    va_end(args);

    fprintf(pFile, "\n");
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
