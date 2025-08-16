#ifndef IPC_HANDLER_H
#define IPC_HANDLER_H

#include "client_context.h"
#include <mqueue.h>
#include <stdbool.h>

#define IPC_QUEUE_NAME "/client_ipc_queue"
#define MAX_MSG_SIZE 256
#define MAX_MSG_COUNT 10

bool ipc_init(ClientContext *context);
bool ipc_send_message(ClientContext *context, const char *message);
int priority_check(const char *message);
bool ipc_exit(ClientContext *context);

// char *ipc_receive_message();

#endif
