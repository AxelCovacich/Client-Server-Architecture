#include "udp_handler.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *udp_listener_thread_func(void *arg) {
    ClientContext *context = (ClientContext *)arg;
    int udp_sock = context->udp_socket;
    char buffer[BUFFER_SIZE_UDP];

    while (1) {
        ssize_t bytes = udp_recv(udp_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes < 0) {
            perror("UDP receive error");
            break; // Exit on error
        }
        if (bytes == 0) {
            printf("UDP socket closed by server.\n");
            break; // Exit if the socket is closed
        }

        buffer[bytes] = '\0';
        printf("[UDP Listener] Message received: %s\n", buffer);
    }
    return NULL;
}

void *keepalive_thread_func(void *arg) {
    ClientContext *context = (ClientContext *)arg;
    int udp_sock = context->udp_socket;
    if (udp_sock < 0) {
        perror("Invalid UDP socket");
        return NULL;
    }
    const char *client_id = client_context_get_id(context);
    if (client_id == NULL || client_id[0] == '\0') {

        fprintf(stderr, "Client ID is not set.\n"); // NOLINT
        return NULL;
    }
    char keepalive_msg[KEEPALIVE_MSG_SIZE];
    snprintf(keepalive_msg, sizeof(keepalive_msg), "{\"command\":\"keepalive\", \"clientId\":\"%s\"}", client_id);

    while (1) {
        printf("\n[Keepalive] Sending heartbeat...\n"); // NOLINT
        udp_send(udp_sock, keepalive_msg, strlen(keepalive_msg), 0);
        sleep(SLEEP_KEEPALIVE_TIME);
    }
    return NULL;
}
