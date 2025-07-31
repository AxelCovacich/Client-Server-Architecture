#include "session_handler.h"
#include "client.h"
#include "input_handler.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Handles a single transaction: sends a message and receives the response.
 * @param sockfd The socket file descriptor.
 * @param message The message to send to the server.
 * @return `TRANSACTION_SUCCESS` if the transaction was successful, `TRANSACTION_ERROR` on any error,
 * `TRANSACTION_SERVER_CLOSED` if the server closes the connection, `TRANSACTION_CLOSE` if client closes connection with
 * end command.
 */
static transaction_result handle_server_transaction(int sockfd, const char *message, recv_fn rx, send_fn tx) {

    if (tx(sockfd, message, strlen(message), 0) < 0) {
        perror("Error writing to socket");
        return TRANSACTION_ERROR;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE); // NOLINT

    ssize_t bytes_read = rx(sockfd, buffer, BUFFER_SIZE - 1, 0);
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
static transaction_result execute_client_action(int sockfd, UserInputAction action, char *buffer, recv_fn rx,
                                                send_fn tx) {

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

        result = handle_server_transaction(sockfd, json_build.json_string, rx, tx);
        free(json_build.json_string);
        return result;
    }

    case INPUT_ACTION_QUIT:

        result = handle_server_transaction(sockfd, "{\"command\":\"end\"}", rx, tx);
        if (result == TRANSACTION_ERROR) {
            return result;
        }
        return TRANSACTION_CLOSE;

    case INPUT_ACTION_CONTINUE:
        return TRANSACTION_SUCCESS;

    case INPUT_ACTION_ERROR:
    default:
        fprintf(stderr, "Invalid input action.\n"); // NOLINT
        return TRANSACTION_SUCCESS;
    }
}

int start_communication(int sockfd, recv_fn rx, send_fn tx) {
    ssize_t bytes_read = 0;
    char buffer[BUFFER_SIZE];

    // one read before while cicle to recieve welcome message from server.
    memset(buffer, '\0', BUFFER_SIZE); // NOLINT

    bytes_read = rx(sockfd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_read <= 0) {
        perror("Failed to receive welcome message or server closed connection");
        return -1;
    }

    buffer[bytes_read] = '\0';
    printf("Server says: %s", buffer);

    // auto login for now
    char hostname[HOSTNAME_BUFFER_SIZE];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Error at gethostname:");
        return -1;
    }

    while (true) {

        printf("Enter message to send(or 'end' to stop): ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {

            break; // EoI
        }
        UserInputAction action = process_user_input(buffer);
        transaction_result result = execute_client_action(sockfd, action, buffer, rx, tx);

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

void *keepalive_thread_func(void *arg) {
    int udp_sock = *(int *)arg;
    const char *keepalive_msg = "{\"command\":\"keepalive\"}";

    while (1) {
        sleep(60);

        printf("\n[Keepalive] Sending heartbeat...\n");
        udp_send(udp_sock, keepalive_msg, strlen(keepalive_msg), 0);
    }
    return NULL;
}