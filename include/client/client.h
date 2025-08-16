#ifndef CLIENT_H
#define CLIENT_H

#include "client_context.h"
#include "input_handler.h"
#include "transport.h"
#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>

// The buffer size is part of the module's public contract
#define BUFFER_SIZE 5048
#define HOSTNAME_BUFFER_SIZE 256
#define LOGIN_JSON_SIZE 512
#define SLEEP_IPC_SECS 2
/**
 * @brief Defines the set of possible results of a single transaction (wirte-read) with the server.
 */
typedef enum {
    TRANSACTION_SUCCESS,
    TRANSACTION_SERVER_CLOSED,
    TRANSACTION_ERROR,
    TRANSACTION_CLOSE,
} transaction_result;

/**
 * @brief Sets up a TCP or UDP connection to a host and port using getaddrinfo.
 * @param context The context containing connection information.
 * @param config The configuration containing host and port information.
 * @param protocol The protocol used to create the socket. Can be `tcp` or `udp`.
 * @return Boolean indicating success or failure of the setup.
 */
bool setup_and_connect(ClientContext *context, client_config config, const char *protocol);

void client_cleanup(ClientContext *context);

bool socket_validation(struct addrinfo *current_addr, ClientContext *context, int sockfd, const char *protocol);

#endif // CLIENT_H