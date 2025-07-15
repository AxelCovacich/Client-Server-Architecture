// src/client/main.c
#include "client.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Main entry of the client.
 *
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments. Expects "./client <host>
 * <port>".
 * @return 0 on successful shutdown, 1 on initialization error.
 */

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Usage: %s host port\n", argv[0]);
        return 1;
    }

    int socket_fd = setup_and_connect(argv[1], argv[2]);

    if (socket_fd == -1) {
        fprintf(stderr, "Could not establish connection.\n");
        return 1;
    }

    printf("Connection successful to %s! Socket FD is %d\n", argv[1], socket_fd);

    if (start_communication(socket_fd) < 0) {

        fprintf(stderr, "An error occurred during communication.\n");
        close(socket_fd);
        return 1;
    }

    close(socket_fd);
    return 0;
}
