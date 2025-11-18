#ifndef CLIENT_CONTEXT_H
#define CLIENT_CONTEXT_H

#include <mqueue.h>
#include <pthread.h>
#include <signal.h>

#define CLIENT_ID_MAX_LEN 64
#define KEEPALIVE_INTERVAL_SEC 60

typedef struct {
    char client_id[CLIENT_ID_MAX_LEN];
    pthread_mutex_t lock;
    int tcp_socket;
    int udp_socket;
    mqd_t ipc_queue; // Message queue for IPC
    volatile sig_atomic_t exit_requested;

} ClientContext;

/**
 * @brief Initializes the client context.
 * @param context A pointer to the ClientContext to initialize.
 */
void client_context_init(ClientContext *context);

/**
 * @brief Sets the client ID in a thread-safe manner.
 * @param context A pointer to the ClientContext.
 * @param id The client ID to set.
 */
void client_context_set_id(ClientContext *context, const char *id);

/**
 * @brief Gets the client ID in a thread-safe manner.
 * @param context A pointer to the ClientContext.
 * @return The client ID string.
 */
const char *client_context_get_id(ClientContext *context);

/**
 * @brief Requests the client context to exit.
 * @param context A pointer to the ClientContext.
 */
void client_context_request_exit(ClientContext *context);

#endif
