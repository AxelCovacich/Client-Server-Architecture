// to store destiny addr
#include "transport.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// ---TCP---
ssize_t tcp_recv(int sockfd, void *buf, size_t len, int flags) {
    (void)flags; // no flags in tcp
    return read(sockfd, buf, len);
}

ssize_t tcp_send(int sockfd, const void *buf, size_t len, int flags) {
    (void)flags;
    return write(sockfd, buf, len);
}

static struct sockaddr_storage peer_addr;
static socklen_t peer_addr_len;

void initialize_udp_peer_address(const struct sockaddr *addr, socklen_t len) {
    if (len <= sizeof(peer_addr)) {
        memcpy(&peer_addr, addr, len);
        peer_addr_len = len;
    }
}

ssize_t udp_recv(int sockfd, void *buf, size_t len, int flags) {
    return recvfrom(sockfd, buf, len, flags, NULL, NULL); // we are reading, don't need src address
}
ssize_t udp_send(int sockfd, const void *buf, size_t len, int flags) {
    return sendto(sockfd, buf, len, flags, (struct sockaddr *)&peer_addr, peer_addr_len);
}