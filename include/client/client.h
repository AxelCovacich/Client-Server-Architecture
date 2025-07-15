#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h> // Para el tipo bool
#include <stddef.h>

// The buffer size is part of the module's public contract
#define BUFFER_SIZE 256

/**
 * @brief Defines the set of possible results of a single transaction (wirte-read) with the server.
 */
typedef enum {
    TRANSACTION_SUCCESS,
    TRANSACTION_SERVER_CLOSED,
    TRANSACTION_ERROR,
} transaction_result;

/**
 * @brief Defines the set of possible actions for the client's main loop.
 *
 * This enumeration is returned by the input processing function to tell the
 * main communication loop what to do next. It allows for a clean, state-based
 * control flow.
 */
typedef enum {
    INPUT_ACTION_SEND,
    INPUT_ACTION_QUIT,
    INPUT_ACTION_CONTINUE,
    INPUT_ACTION_ERROR,
} UserInputAction;

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

/**
 * @brief Processes the user's raw input and determines the next action.
 * @param buffer The buffer containing input from fgets.
 * @return A UserInputAction enum value indicating what the main loop should do.
 */
UserInputAction process_user_input(char *buffer);

/**
 * @brief Safely builds a message string in a destination buffer.
 *
 * This function uses snprintf to format content into a buffer, which prevents
 * buffer overflows by respecting the provided size limit. It is the designated
 * function for preparing all outgoing messages.
 *
 * @param dest_buffer The destination buffer where the final message will be written.
 * @param size The total size of the destination buffer.
 * @param content The source content string to format into the buffer.
 * @return The number of characters that would have been written (excluding the
 * null terminator), or a negative value on an encoding error.
 */
int build_message(char *dest_buffer, size_t size, const char *content);

#endif // CLIENT_H