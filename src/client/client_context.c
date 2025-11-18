#include "client_context.h"
#include <string.h>

void client_context_init(ClientContext *context) {
    memset(context->client_id, 0, CLIENT_ID_MAX_LEN); // NOLINT
    pthread_mutex_init(&context->lock, NULL);
    context->tcp_socket = -1;
    context->udp_socket = -1;
    context->ipc_queue = (mqd_t)-1; // Initialize message queue to an invalid value
    context->exit_requested = 0;
}

void client_context_set_id(ClientContext *context, const char *client_id) {
    pthread_mutex_lock(&context->lock);
    if (client_id == NULL) {
        context->client_id[0] = '\0';
    } else {
        strncpy(context->client_id, client_id, CLIENT_ID_MAX_LEN - 1);
        context->client_id[CLIENT_ID_MAX_LEN - 1] = '\0'; // Ensure null termination
    }
    pthread_mutex_unlock(&context->lock);
}

const char *client_context_get_id(ClientContext *context) {

    pthread_mutex_lock(&context->lock);
    const char *tempid = context->client_id;
    pthread_mutex_unlock(&context->lock);
    return tempid;
}

void client_context_request_exit(ClientContext *context) {
    context->exit_requested = 1;
}
