
// src/client/client.c
#define _POSIX_C_SOURCE 200112L // Necessary to use getaddrinfo

#include "client.h"
#include "ipc_handler.h"
#include "logger.h"
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

bool setup_and_connect(ClientContext *context, client_config config, const char *protocol) {
    struct addrinfo hints;
    struct addrinfo *result_list = NULL;
    struct addrinfo *current_addr = NULL;
    int sockfd = -1;
    int getaddrinfo_status = 1;
    const char *port = NULL;
    const char *host = config.host;

    memset(&hints, 0, sizeof(hints)); // NOLINT
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    if (strcmp(protocol, "tcp") == 0) {

        hints.ai_socktype = SOCK_STREAM; // TCP
        port = config.port_tcp;
    } else {

        hints.ai_socktype = SOCK_DGRAM; // UDP
        port = config.port_udp;
    }

    // we use getaddrinfo(thread safe and supports Ipv4 and Ipv6) instead of
    // gethostbyname (obsolet)
    getaddrinfo_status = getaddrinfo(host, port, &hints, &result_list);
    if (getaddrinfo_status != 0) {
        logger_log("Client", ERROR, ("getaddrinfo error: %s", gai_strerror(getaddrinfo_status)));
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(getaddrinfo_status)); // NOLINT
        return false;
    }

    for (current_addr = result_list; current_addr != NULL; current_addr = current_addr->ai_next) {

        sockfd = socket(current_addr->ai_family, current_addr->ai_socktype, current_addr->ai_protocol);
        if (sockfd == -1) {
            logger_log("Client", ERROR, ("socket error: %s", strerror(errno)));
            perror("client: socket");
            continue;
        }

        if (strcmp(protocol, "udp") == 0) {
            initialize_udp_peer_address(current_addr->ai_addr, current_addr->ai_addrlen);
        }

        if (hints.ai_socktype == SOCK_STREAM) {
            if (connect(sockfd, current_addr->ai_addr, current_addr->ai_addrlen) == -1) {
                logger_log("Client", ERROR, ("connect error: %s", strerror(errno)));
                perror("client: connect");
                close(sockfd);
                sockfd = -1;
                continue;
            }
        }
        break;
    }

    freeaddrinfo(result_list);

    return socket_validation(current_addr, context, sockfd, protocol);
}

void client_cleanup(ClientContext *context) {
    if (context->tcp_socket != -1) {
        close(context->tcp_socket);
    }
    if (context->udp_socket != -1) {
        close(context->udp_socket);
    }
    if (context->ipc_queue != (mqd_t)-1) {
        ipc_exit(context);
        sleep(SLEEP_IPC_SECS); // Give some time for the IPC to exit gracefully
        mq_close(context->ipc_queue);
        mq_unlink(IPC_QUEUE_NAME);
    }
    logger_close();
    printf("Client cleanup completed.\n"); // NOLINT
}

bool socket_validation(struct addrinfo *current_addr, ClientContext *context, int sockfd, const char *protocol) {
    if (current_addr != NULL) {
        if (strcmp(protocol, "tcp") == 0) {
            context->tcp_socket = sockfd;
        } else {
            context->udp_socket = sockfd;
        }
        logger_log("Client", INFO,
                   (strcmp(protocol, "tcp") == 0) ? "TCP connection established." : "UDP connection established.");
        return true;
    }
    // If we reach here, it means no valid address was found
    logger_log("Client", ERROR, "Failed to connect to any address.");
    fprintf(stderr, "Failed to connect to any address.\n"); // NOLINT
    close(sockfd);
    return false;
}
