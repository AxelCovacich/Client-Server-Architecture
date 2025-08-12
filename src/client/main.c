// src/client/main.c
#include "client.h"
#include "client_context.h"
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

    ClientContext context;
    client_context_init(&context);
    context.tcp_socket = setup_and_connect(config.host, config.port_tcp, "tcp");

    if (context.tcp_socket == -1) {
        fprintf(stderr, "Could not establish tcp connection.\n"); // NOLINT
        return 1;
    }

    printf("Connection successful to %s! TCP Socket FD is %d\n", argv[1], context.tcp_socket);

    context.udp_socket = setup_and_connect(config.host, config.port_udp, "udp");
    if (context.udp_socket == -1) {
        fprintf(stderr, "Could not establish udp connection.\n"); // NOLINT
        return 1;
    }
    printf("Connection successful to %s! UDP Socket FD is %d\n", argv[1], context.udp_socket);

    if (start_communication(&context, tcp_recv, tcp_send) < 0) {

        fprintf(stderr, "An error occurred during communication.\n"); // NOLINT
        close(context.tcp_socket);
        return 1;
    }

    close(context.tcp_socket);
    close(context.udp_socket);
    return 0;
}
