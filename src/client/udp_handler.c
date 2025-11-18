#include "udp_handler.h"
#include "errno.h"
#include "ipc_handler.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *udp_listener_thread_func(void *arg) {
    ClientContext *context = (ClientContext *)arg;
    int udp_sock = context->udp_socket;
    char buffer[BUFFER_SIZE_UDP];

    while (context->exit_requested == 0) {
        ssize_t bytes = udp_recv(udp_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes < 0) {
            if (context->exit_requested != 0) {
                break; // Just exit if requested and just wake up from recv
            }
            logger_log("UDP_handler", ERROR, "UDP receive error: %s", strerror(errno));
            perror("UDP receive error");
            break; // Exit on error
        }
        if (bytes == 0) {
            logger_log("UDP_handler", WARNING, "UDP socket closed by server.");
            printf("UDP socket closed by server.\n");
            break; // Exit if the socket is closed
        }

        buffer[bytes] = '\0';
        logger_log("UDP_handler", INFO, "UDP Message received: %s", buffer);
        if (!ipc_send_message(context, buffer)) {
            logger_log("UDP_handler", ERROR, "Error: Failed to send IPC message.");
            fprintf(stderr, "Error: Failed to send IPC message.\n"); // NOLINT
        }
    }
    return NULL;
}

void *keepalive_thread_func(void *arg) {
    ClientContext *context = (ClientContext *)arg;
    int udp_sock = context->udp_socket;
    if (udp_sock < 0) {
        logger_log("UDP_handler", ERROR, "Invalid UDP socket: %s", strerror(errno));
        perror("Invalid UDP socket");
        return NULL;
    }
    const char *client_id = client_context_get_id(context);
    if (client_id == NULL || client_id[0] == '\0') {
        logger_log("UDP_handler", WARNING, "Client ID is not set.");
        fprintf(stderr, "Client ID is not set.\n"); // NOLINT
        return NULL;
    }
    char keepalive_msg[KEEPALIVE_MSG_SIZE];
    snprintf(keepalive_msg, sizeof(keepalive_msg), "{\"command\":\"keepalive\", \"clientId\":\"%s\"}", // NOLINT
             client_id);

    while (context->exit_requested == 0) {

        if (udp_send(udp_sock, keepalive_msg, strlen(keepalive_msg), 0) < 0) {
            logger_log("UDP_handler", ERROR, "Failed to send keepalive message: %s", strerror(errno));
            perror("Failed to send keepalive message");
            break; // Exit on error
        }
        logger_log("UDP_handler", INFO, "Keepalive message sent successfully.");
        sleep(SLEEP_KEEPALIVE_TIME);
    }
    return NULL;
}
