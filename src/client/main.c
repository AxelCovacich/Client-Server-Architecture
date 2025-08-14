// src/client/main.c
#include "client.h"
#include "client_context.h"
#include "input_handler.h"
#include "ipc_handler.h"
#include "logger.h"
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

    if (logger_init(LOG_PATH) != 0) {
        fprintf(stderr, "Logger initialization failed. Exiting.\n"); // NOLINT
        return 1;
    }
    ClientContext context;
    client_context_init(&context);

    bool setup = setup_and_connect(&context, config, "tcp");
    if (setup == false) {
        logger_log("Main", ERROR, "Could not establish TCP connection.");
        fprintf(stderr, "Could not establish tcp connection.\n"); // NOLINT
        client_cleanup(&context);
        return 1;
    }
    printf("Connection successful to %s! TCP Socket FD is %d\n", argv[1], context.tcp_socket);

    bool udp_setup = setup_and_connect(&context, config, "udp");
    if (udp_setup == false) {
        logger_log("Main", ERROR, "Could not establish UDP connection.");
        client_cleanup(&context);
        fprintf(stderr, "Could not establish udp connection.\n"); // NOLINT
        return 1;
    }
    printf("Connection successful to %s! UDP Socket FD is %d\n", argv[1], context.udp_socket);

    if (!ipc_init(&context)) {
        logger_log("Main", ERROR, "IPC initialization failed.");
        fprintf(stderr, "IPC initialization failed.\n"); // NOLINT
        client_cleanup(&context);
        return 1;
    }
    if (start_communication(&context, tcp_recv, tcp_send) < 0) {
        logger_log("Main", ERROR, "Communication failed.");
        fprintf(stderr, "An error occurred during communication.\n"); // NOLINT
        client_cleanup(&context);
        return 1;
    }

    client_cleanup(&context);
    return 0;
}
