#ifndef IPC_HANDLER_H
#define IPC_HANDLER_H

#include "client_context.h"
#include <mqueue.h>
#include <stdbool.h>

#define IPC_QUEUE_NAME "/client_ipc_queue"
#define MAX_MSG_SIZE 256
#define MAX_MSG_COUNT 10

/**
 * @brief Initializes the IPC message queue for the client.
 * @param context A pointer to the ClientContext to initialize IPC for.
 * @return True on success, false on failure.
 */
bool ipc_init(ClientContext *context);

/**
 * @brief Sends a message to the IPC queue.
 * @param context A pointer to the ClientContext containing the IPC queue.
 * @param message The message string to send.
 * @return True on success, false on failure.
 */
bool ipc_send_message(ClientContext *context, const char *message);

/**
 * @brief Checks the priority of the message based on its content.
 * @param message The message string to check.
 * @return The priority level (0-3) or -1 on error.
 */
int priority_check(const char *message);

/**
 * @brief Cleans up the IPC message queue.
 * @param context A pointer to the ClientContext to clean up IPC for.
 * @return True on success, false on failure.
 */
bool ipc_exit(ClientContext *context);

#endif
