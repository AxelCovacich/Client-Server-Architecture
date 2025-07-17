
// src/client/client.c
#define _POSIX_C_SOURCE 200112L // Necessary to use getaddrinfo

#include "client.h"
#include "input_handler.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief Handles a single transaction: sends a message and receives the response.
 * @param sockfd The socket file descriptor.
 * @param message The message to send to the server.
 * @return `TRANSACTION_SUCCESS` if the transaction was successful, `TRANSACTION_ERROR` on any error,
 * `TRANSACTION_SERVER_CLOSED` if the server closes the connection, `TRANSACTION_CLOSE` if client closes connection with
 * end command.
 */
static transaction_result handle_server_transaction(int sockfd, const char *message) {

    if (write(sockfd, message, strlen(message)) < 0) {
        perror("Error writing to socket");
        return TRANSACTION_ERROR;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    ssize_t bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Error reading from socket");
        return TRANSACTION_ERROR;
    }
    if (bytes_read == 0) {
        printf("\nServer closed the connection.\n");
        return TRANSACTION_SERVER_CLOSED;
    }
    buffer[bytes_read] = '\0';
    printf("Answer from server: %s\n", buffer);
    return TRANSACTION_SUCCESS;
}

/**
 * @brief Executes a specific client action based on parsed user input.
 *
 * This function acts as a dispatcher for the main communication loop. It takes
 * a UserInputAction and performs the corresponding task, such as building and
 * sending a JSON message or handling the quit sequence.
 *
 * @param sockfd The active socket file descriptor for the server connection.
 * @param action The UserInputAction enum value determining which action to perform.
 * @param buffer The raw user input buffer, used to build the JSON message for SEND actions.
 * @return A TransactionResult enum value indicating the outcome of the operation.
 */
static transaction_result execute_client_action(int sockfd, UserInputAction action, char *buffer) {

    transaction_result result = TRANSACTION_SUCCESS;
    switch (action) {
    case INPUT_ACTION_SEND: {
        json_build_result json_build = build_json_from_input(buffer);
        if (json_build.status == JSON_BUILD_ERROR_MEMORY) {
            return TRANSACTION_ERROR;
        }
        if (json_build.status == JSON_BUILD_ERROR_SYNTAX) {
            return TRANSACTION_SUCCESS; // Not sending anything, going back to loop for another user input
        }

        result = handle_server_transaction(sockfd, json_build.json_string);
        free(json_build.json_string);
        return result;
    }

    case INPUT_ACTION_QUIT:

        result = handle_server_transaction(sockfd, "{\"command\":\"end\"}");
        if (result == TRANSACTION_ERROR) {
            return result;
        }
        return TRANSACTION_CLOSE;

    case INPUT_ACTION_CONTINUE:
        return TRANSACTION_SUCCESS;

    case INPUT_ACTION_ERROR:
    default:
        fprintf(stderr, "Invalid input action.\n");
        return TRANSACTION_SUCCESS;
    }
}

int setup_and_connect(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *result_list, *p;
    int sockfd = -1;
    int getaddrinfo_status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    // we use getaddrinfo(thread safe and supports Ipv4 and Ipv6) instead of
    // gethostbyname (obsolet)
    getaddrinfo_status = getaddrinfo(host, port, &hints, &result_list);
    if (getaddrinfo_status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(getaddrinfo_status));
        return -1;
    }

    for (p = result_list; p != NULL; p = p->ai_next) {

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {

            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {

            perror("client: connect");
            close(sockfd);
            sockfd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(result_list);

    return (p == NULL) ? -1 : sockfd;
}

int start_communication(int sockfd) {
    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];

    // one read before while cicle to recieve welcome message from server.
    memset(buffer, '\0', BUFFER_SIZE);
    bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);

    if (bytes_read <= 0) {
        perror("Failed to receive welcome message or server closed connection");
        return -1;
    }

    buffer[bytes_read] = '\0';
    printf("Server says: %s", buffer);

    while (true) {

        printf("Enter message to send(or 'end' to stop): ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {

            break; // EoI
        }
        UserInputAction action = process_user_input(buffer);
        transaction_result result = execute_client_action(sockfd, action, buffer);

        if (result == TRANSACTION_ERROR) {
            printf("Closing client...\n");
            return -1;
        }
        if (result == TRANSACTION_SERVER_CLOSED || result == TRANSACTION_CLOSE) {
            break;
        }
    }
    printf("Closing client...\n");
    return 0;
}
