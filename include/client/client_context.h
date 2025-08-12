#ifndef CLIENT_CONTEXT_H
#define CLIENT_CONTEXT_H

#include <pthread.h>

#define CLIENT_ID_MAX_LEN 64
#define KEEPALIVE_INTERVAL_SEC 60

typedef struct {
    char client_id[CLIENT_ID_MAX_LEN];
    pthread_mutex_t lock;
    int tcp_socket;
    int udp_socket;
} ClientContext;

void client_context_init(ClientContext *context);
void client_context_set_id(ClientContext *context, const char *id);
const char *client_context_get_id(ClientContext *context);

#endif
