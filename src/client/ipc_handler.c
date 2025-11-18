#include "ipc_handler.h"
#include "cJSON.h"
#include "errno.h"
#include "logger.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

bool ipc_init(ClientContext *context) {
    struct mq_attr attr;
    attr.mq_flags = 0;              // Flags: 0 for blocking, O_NONBLOCK for non-blocking
    attr.mq_maxmsg = MAX_MSG_COUNT; // Maximum number of messages in queue
    attr.mq_msgsize = MAX_MSG_SIZE; // Maximum message size (bytes)
    attr.mq_curmsgs = 0;            // Number of messages currently in queue

    mqd_t message_queue = mq_open(IPC_QUEUE_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
                                  &attr); // 0644  Permissions: rw-r--r--
    if (message_queue == (mqd_t)-1) {
        perror("mq_open");
        logger_log("IPC_handler", ERROR, "Failed to create/open IPC message queue: %s", strerror(errno));
        return false;
    }

    context->ipc_queue = message_queue;
    return true;
}

bool ipc_send_message(ClientContext *context, const char *message) {
    if (context->ipc_queue == (mqd_t)-1) {
        fprintf(stderr, "IPC queue not initialized.\n"); // NOLINT
        logger_log("IPC_handler", ERROR, "IPC queue not initialized.");
        return false;
    }
    if (strlen(message) >= MAX_MSG_SIZE) {
        fprintf(stderr, "Message too long for IPC queue.\n"); // NOLINT
        logger_log("IPC_handler", ERROR, "Message too long for IPC queue.");
        return false;
    }

    int priority = priority_check(message);
    if (priority < 0) {
        fprintf(stderr, "Invalid message format for IPC queue.\n"); // NOLINT
        logger_log("IPC_handler", ERROR, "Invalid message format for IPC queue.");
        return false;
    }

    if (mq_send(context->ipc_queue, message, strlen(message) + 1, priority) == -1) {
        if (errno == EAGAIN) {
            fprintf(stderr, "IPC queue full, message dropped.\n");
            logger_log("IPC_handler", WARNING, "IPC queue full, message dropped.");
        } else {
            perror("mq_send");
            logger_log("IPC_handler", ERROR, "Failed to send message to IPC queue: %s", strerror(errno));
        }
        return false;
    }
    logger_log("IPC_handler", INFO, "Message sent to IPC queue successfully.");
    return true;
}

int priority_check(const char *message) {
    cJSON *root = cJSON_Parse(message);
    if (root == NULL) {
        fprintf(stderr, "Error: Could not parse IPC message.\n"); // NOLINT
        logger_log("IPC_handler", ERROR, "Could not parse IPC message.");
        return -1;
    }

    cJSON *category = cJSON_GetObjectItem(root, "category");
    if (category == NULL || !cJSON_IsString(category)) {
        fprintf(stderr, "Error: Invalid IPC message (missing category).\n"); // NOLINT
        logger_log("IPC_handler", ERROR, "Invalid IPC message (missing category).");
        cJSON_Delete(root);
        return -1;
    }

    int priority = 0; // Default priority
    if (strcmp(category->valuestring, "keepalive") == 0) {
        priority = 1;
    }
    if (strcmp(category->valuestring, "alert") == 0) {
        priority = 2; // Higher priority for alerts
    }
    if (strcmp(category->valuestring, "exit") == 0) {
        priority = 3; // Highest priority for exit messages
    }

    cJSON_Delete(root);
    return priority;
}

bool ipc_exit(ClientContext *context) {
    cJSON *message = cJSON_CreateObject();
    if (message == NULL) {
        fprintf(stderr, "Error creating IPC exit message.\n"); // NOLINT
        logger_log("IPC_handler", ERROR, "Error creating IPC exit message.");
        return false;
    }
    cJSON_AddStringToObject(message, "category", "exit");
    cJSON_AddStringToObject(message, "message", "Closing client, goodbye!");
    char *messageString = cJSON_Print(message);
    bool result = ipc_send_message(context, messageString);
    cJSON_Delete(message);
    free(messageString);
    return result;
}
