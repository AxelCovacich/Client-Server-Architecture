#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdio.h>
#include <sys/socket.h>

typedef ssize_t (*recv_fn)(int sockfd, void *buf, size_t len, int flags);
typedef ssize_t (*send_fn)(int sockfd, const void *buf, size_t len, int flags);

// --- TCP ---
ssize_t tcp_recv(int sockfd, void *buf, size_t len, int flags);
ssize_t tcp_send(int sockfd, const void *buf, size_t len, int flags);

// --- UDP ---

/// @brief This is the setter for the adress to use in udp protocol
/// @param addr server adress
/// @param len length of server adress
void initialize_udp_peer_address(const struct sockaddr *addr, socklen_t len);

ssize_t udp_recv(int sockfd, void *buf, size_t len, int flags);
ssize_t udp_send(int sockfd, const void *buf, size_t len, int flags);

#endif // TRANSPORT_H