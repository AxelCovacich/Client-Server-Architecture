#ifndef CLIENT_H
#define CLIENT_H

#include "client_context.h"
#include "transport.h"
#include <stdbool.h> // Para el tipo bool
#include <stddef.h>

// The buffer size is part of the module's public contract
#define BUFFER_SIZE 5048
#define HOSTNAME_BUFFER_SIZE 256
#define LOGIN_JSON_SIZE 512

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
 * @brief Sets up a TCP connection to a host and port using getaddrinfo.
 * @param host The hostname or IP address of the server.
 * @param port The port number as a string (e.g., "8080").
 * @param protocol The protocol used to create the socket. Can be `tcp` or `udp`.
 * @return The socket file descriptor on success, or -1 on failure.
 */
int setup_and_connect(const char *host, const char *port, const char *protocol);

void client_cleanup(ClientContext *context);

#endif // CLIENT_H