
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

int setup_and_connect(const char *host, const char *in_port, const char *protocol) {
    struct addrinfo hints;
    struct addrinfo *result_list = NULL;
    struct addrinfo *current_addr = NULL;
    int sockfd = -1;
    int getaddrinfo_status = 1;
    const char *port = in_port;

    memset(&hints, 0, sizeof(hints)); // NOLINT
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    if (strcmp(protocol, "tcp") == 0) {

        hints.ai_socktype = SOCK_STREAM; // TCP
    } else {

        hints.ai_socktype = SOCK_DGRAM; // UDP
    }

    // we use getaddrinfo(thread safe and supports Ipv4 and Ipv6) instead of
    // gethostbyname (obsolet)
    getaddrinfo_status = getaddrinfo(host, port, &hints, &result_list);
    if (getaddrinfo_status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(getaddrinfo_status)); // NOLINT
        return -1;
    }

    for (current_addr = result_list; current_addr != NULL; current_addr = current_addr->ai_next) {

        sockfd = socket(current_addr->ai_family, current_addr->ai_socktype, current_addr->ai_protocol);
        if (sockfd == -1) {

            perror("client: socket");
            continue;
        }

        if (strcmp(protocol, "udp") == 0) {
            initialize_udp_peer_address(current_addr->ai_addr, current_addr->ai_addrlen);
            udp_send(sockfd, "", 0, 0); // check in for server to know client addr
        }

        if (hints.ai_socktype == SOCK_STREAM) {
            if (connect(sockfd, current_addr->ai_addr, current_addr->ai_addrlen) == -1) {

                perror("client: connect");
                close(sockfd);
                sockfd = -1;
                continue;
            }
        }
        break;
    }

    freeaddrinfo(result_list);

    return (current_addr == NULL) ? -1 : sockfd;
}
