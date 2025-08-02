// src/client/main.c
#include "client.h"
#include "input_handler.h"
#include "session_handler.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <transport.h>
#include <unistd.h>

/**
 * @brief Main entry of the client.
 *
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments. Expects "./client <host> <port>
 * <port>".
 * @return 0 on successful shutdown, 1 on initialization error.
 */

int main(int argc, const char *argv[]) {
    client_config config;
    if (!parse_arguments(argc, argv, &config)) {
        return 1;
    }

    int socket_tcp_fd = setup_and_connect(config.host, config.port_tcp, "tcp");

    if (socket_tcp_fd == -1) {
        fprintf(stderr, "Could not establish tcp connection.\n"); // NOLINT
        return 1;
    }

    printf("Connection successful to %s! TCP Socket FD is %d\n", argv[1], socket_tcp_fd);

    int socket_udp_fd = setup_and_connect(config.host, config.port_udp, "udp");
    if (socket_tcp_fd == -1) {
        fprintf(stderr, "Could not establish udp connection.\n"); // NOLINT
        return 1;
    }
    printf("Connection successful to %s! UDP Socket FD is %d\n", argv[1], socket_udp_fd);

    pthread_t keepalive_thread = 0;
    if (pthread_create(&keepalive_thread, NULL, keepalive_thread_func, &socket_udp_fd) != 0) {
        close(socket_udp_fd);
        perror("Failed to create keepalive thread");
        return 1;
    }

    if (start_communication(socket_tcp_fd, tcp_recv, tcp_send) < 0) {

        fprintf(stderr, "An error occurred during communication.\n"); // NOLINT
        close(socket_tcp_fd);
        return 1;
    }

    close(socket_tcp_fd);
    close(socket_udp_fd);
    return 0;
}
