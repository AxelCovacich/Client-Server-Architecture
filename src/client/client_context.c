#include "client_context.h"
#include <string.h>

void client_context_init(ClientContext *context) {
    memset(context->client_id, 0, CLIENT_ID_MAX_LEN); // NOLINT
    pthread_mutex_init(&context->lock, NULL);
    context->tcp_socket = -1;
    context->udp_socket = -1;
    context->ipc_queue = (mqd_t)-1; // Initialize message queue to an invalid value
}

void client_context_set_id(ClientContext *context, const char *client_id) {
    pthread_mutex_lock(&context->lock);
    strncpy(context->client_id, client_id, CLIENT_ID_MAX_LEN - 1); // NOLINT
    pthread_mutex_unlock(&context->lock);
}

const char *client_context_get_id(ClientContext *context) {
    pthread_mutex_lock(&context->lock);
    static char temp[CLIENT_ID_MAX_LEN];
    strncpy(temp, context->client_id, CLIENT_ID_MAX_LEN - 1); // NOLINT
    pthread_mutex_unlock(&context->lock);
    return temp;
}
