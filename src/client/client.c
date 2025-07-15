
// src/client/client.c
#define _POSIX_C_SOURCE 200112L // Necessary to use getaddrinfo

#include "client.h"
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
 * @return `true` if the transaction was successful, `false` on any error or if the server closes the connection.
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
    transaction_result t_result = TRANSACTION_SUCCESS;

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

            return 0; // EoI
        }
        UserInputAction action = process_user_input(buffer);
        switch (action) {
        case INPUT_ACTION_SEND:
            // nothing to do here, just write
            // build_message(buffer, BUFFER_SIZE, buffer);
            t_result = handle_server_transaction(sockfd, buffer);
            break;

        case INPUT_ACTION_QUIT:
            build_message(buffer, BUFFER_SIZE, buffer);
            t_result = handle_server_transaction(sockfd, "end");
            return 0;

        case INPUT_ACTION_CONTINUE:
            continue;
        }

        if (t_result == TRANSACTION_ERROR) {
            return -1;
        }
        if (t_result == TRANSACTION_SERVER_CLOSED) {
            return 0;
        }
    }
    printf("Closing client...\n");
    return 0;
}

UserInputAction process_user_input(char *buffer) {

    buffer[strcspn(buffer, "\n")] = 0;

    if (strcmp("end", buffer) == 0) {
        return INPUT_ACTION_QUIT;
    }
    if (strlen(buffer) == 0) {
        return INPUT_ACTION_CONTINUE;
    }
    return INPUT_ACTION_SEND;
}

int build_message(char *dest_buffer, size_t size, const char *content) {

    int written_bytes = snprintf(dest_buffer, size, "%s", content);
    if (written_bytes < 0 || written_bytes >= size) {
        // Handle error or truncation case, for now just report
        fprintf(stderr, "Warning: message was truncated or an error occurred.\n");
    }
    return written_bytes;
}