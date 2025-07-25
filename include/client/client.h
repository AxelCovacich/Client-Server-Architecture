#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h> // Para el tipo bool
#include <stddef.h>

// The buffer size is part of the module's public contract
#define BUFFER_SIZE 256
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
 * @return The socket file descriptor on success, or -1 on failure.
 */
int setup_and_connect(const char *host, const char *port);

/**
 * @brief Manages the communication loop with the server.
 * * Reads user input from stdin, sends it to the server, and prints the
 * server's response. The loop terminates if the user types "end", if the
 * connection is closed, or if an error occurs.
 * @param sockfd The socket file descriptor connected to the server.
 * @return Returns 0 on a clean shutdown, or -1 on a communication error.
 */
int start_communication(int sockfd);

#endif // CLIENT_H