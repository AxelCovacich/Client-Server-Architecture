// session_handler.c

#include "session_handler.h"
#include "errno.h"
#include "logger.h"
#include "output_handler.h"
#include "udp_handler.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

volatile sig_atomic_t exit_requested = 0;

/**
 * @brief Handles a single transaction: sends a message and receives the response.
 * @param sockfd The socket file descriptor.
 * @param message The message to send to the server.
 * @return `TRANSACTION_SUCCESS` if the transaction was successful, `TRANSACTION_ERROR` on any error,
 * `TRANSACTION_SERVER_CLOSED` if the server closes the connection, `TRANSACTION_CLOSE` if client closes connection with
 * end command.
 */
static transaction_result handle_server_transaction(ClientContext *context, const char *message, recv_fn recieve,
                                                    send_fn send, const char *input_buffer_copy) {

    if (send(context->tcp_socket, message, strlen(message), 0) < 0) {
        perror("Error writing to socket");
        logger_log("Session_handler", ERROR, ("Error writing to socket: %s", strerror(errno)));
        return TRANSACTION_ERROR;
    }

    char server_response[BUFFER_SIZE];
    memset(server_response, '\0', BUFFER_SIZE); // NOLINT

    ssize_t bytes_read = recieve(context->tcp_socket, server_response, BUFFER_SIZE - 1, 0);
    if (bytes_read < 0) {
        perror("Error reading from socket");
        logger_log("Session_handler", ERROR, ("Error reading from socket: %s", strerror(errno)));
        return TRANSACTION_ERROR;
    }
    if (bytes_read == 0) {
        printf("\nServer closed the connection.\n");
        logger_log("Session_handler", WARNING, "Server closed the connection.");
        return TRANSACTION_SERVER_CLOSED;
    }
    server_response[bytes_read] = '\0';

    print_readable_response(context, server_response, input_buffer_copy, stdout);
    // printf("Answer from server: %s\n", buffer);
    logger_log("Session_handler", INFO, "Successfull Transaction. Received response from server");
    return TRANSACTION_SUCCESS;
}

transaction_result execute_client_action(ClientContext *context, UserInputAction action, char *buffer, recv_fn recieve,
                                         send_fn send) {

    transaction_result result = TRANSACTION_SUCCESS;
    switch (action) {
    case INPUT_ACTION_SEND: {
        const char *input_buffer_copy = buffer;
        json_build_result json_build = build_json_from_input(buffer);
        if (json_build.status == JSON_BUILD_ERROR_MEMORY) {
            logger_log("Session_handler", ERROR, "Memory error while building JSON.");
            return TRANSACTION_ERROR;
        }
        if (json_build.status == JSON_BUILD_ERROR_SYNTAX) {
            return TRANSACTION_SUCCESS; // Not sending anything, going back to loop for another user input
        }

        result = handle_server_transaction(context, json_build.json_string, recieve, send, input_buffer_copy);
        free(json_build.json_string);
        return result;
    }

    case INPUT_ACTION_QUIT:

        result = handle_server_transaction(context, "{\"command\":\"end\"}", recieve, send, "end");
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

int start_communication(ClientContext *context, recv_fn recieve, send_fn send) {
    ssize_t bytes_read = 0;
    char buffer[BUFFER_SIZE];

    // one read before while cicle to recieve welcome message from server.
    memset(buffer, '\0', BUFFER_SIZE); // NOLINT

    bytes_read = recieve(context->tcp_socket, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_read <= 0) {
        logger_log("Session_handler", ERROR,
                   ("Failed to receive welcome message or server closed connection: %s", strerror(errno)));
        perror("Failed to receive welcome message or server closed connection");
        return -1;
    }

    buffer[bytes_read] = '\0';
    printf("Server says: %s", buffer);

    while (exit_requested == 0) {

        printf("Enter message to send(or 'end' to stop): ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {

            break; // EoI
        }
        UserInputAction action = process_user_input(buffer);
        transaction_result result = execute_client_action(context, action, buffer, recieve, send);

        if (result == TRANSACTION_ERROR) {
            logger_log("Session_handler", ERROR, "Error during transaction execution");
            printf("Closing client...\n");
            return -1;
        }
        if (result == TRANSACTION_SERVER_CLOSED || result == TRANSACTION_CLOSE) {
            break;
        }
    }
    logger_log("Session_handler", INFO, "Client session ended by user or server.");
    printf("\nClosing client...\n");
    return 0;
}

void session_start_aux_threads(ClientContext *context) {

    pthread_t keepalive_thread = 0;
    if (pthread_create(&keepalive_thread, NULL, keepalive_thread_func, context) != 0) {
        logger_log("Session_handler", ERROR, ("Failed to create keepalive thread: %s", strerror(errno)));
        perror("Failed to create keepalive thread");
        return;
    }
    pthread_detach(keepalive_thread); // thread will run independently and will not block the main thread. At
                                      // finish, it will clean up itself.

    pthread_t udp_listener_thread = 0;
    if (pthread_create(&udp_listener_thread, NULL, udp_listener_thread_func, context) != 0) {
        logger_log("Session_handler", ERROR, ("Failed to create UDP listener thread: %s", strerror(errno)));
        perror("Failed to create UDP listener thread");
        return;
    }
    pthread_detach(udp_listener_thread);
}

void launch_dashboard(ClientContext *context) {

    pid_t pid = fork();
    if (pid == 0) {
        // Child process → runs dashboard
        prctl(PR_SET_PDEATHSIG, SIGTERM); // Ensure child terminates if parent dies abruptly
        char *args[] = {"python3", "./scripts/dashboard.py", client_context_get_id(context), NULL};
        execvp("python3", args);
        perror("execvp failed");
        _exit(1); // Safe exit if exec fails
    } else if (pid > 0) {
        // Parent process → continues as if nothing happened
        // We don't call wait(), the child is independent
    } else {
        perror("fork failed");
        logger_log("Session_handler", ERROR, ("Failed to fork for dashboard: %s", strerror(errno)));
    }
    logger_log("Session_handler", INFO, "Dashboard launched successfully.");
}

void signal_handler(int signum) {
    exit_requested = 1;
    fclose(stdin); // EoF to break fgets loop
}