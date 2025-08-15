#include "ipc_handler.h"
#include "cJSON.h"
#include "errno.h"
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

    mqd_t mq = mq_open(IPC_QUEUE_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
                       &attr); // 0644  Permissions: rw-r--r--
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return false;
    }

    context->ipc_queue = mq;
    return true;
}

void ipc_send_message(ClientContext *context, const char *message) {
    if (context->ipc_queue == (mqd_t)-1) {
        fprintf(stderr, "IPC queue not initialized.\n"); // NOLINT
        return;
    }
    if (strlen(message) >= MAX_MSG_SIZE) {
        fprintf(stderr, "Message too long for IPC queue.\n"); // NOLINT
        return;
    }

    int priority = priority_check(message);
    if (priority < 0) {
        fprintf(stderr, "Invalid message format for IPC queue.\n"); // NOLINT
        return;
    }

    if (mq_send(context->ipc_queue, message, strlen(message) + 1, priority) == -1) {
        if (errno == EAGAIN) {
            fprintf(stderr, "IPC queue full, message dropped.\n");
        } else {
            perror("mq_send");
        }
        return;
    }
}

int priority_check(const char *message) {
    cJSON *root = cJSON_Parse(message);
    if (root == NULL) {
        fprintf(stderr, "Error: Could not parse IPC message.\n"); // NOLINT
        return -1;
    }

    cJSON *category = cJSON_GetObjectItem(root, "category");
    if (category == NULL || !cJSON_IsString(category)) {
        fprintf(stderr, "Error: Invalid IPC message (missing category).\n"); // NOLINT
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

void ipc_exit(ClientContext *context) {
    cJSON *message = cJSON_CreateObject();
    if (message == NULL) {
        fprintf(stderr, "Error creating IPC exit message.\n"); // NOLINT
        return;
    }
    cJSON_AddStringToObject(message, "category", "exit");
    cJSON_AddStringToObject(message, "message", "Closing client, goodbye!");
    ipc_send_message(context, cJSON_Print(message));
    cJSON_Delete(message);
}