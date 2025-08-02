/**
 * @file transport.h
 * @brief Defines an abstraction layer for network transport protocols (TCP/UDP).
 *
 * This module provides a generic interface for sending and receiving data,
 * allowing the core client logic to be independent of the underlying transport protocol.
 * @author Axel Covacich
 * @copyright Copyright (c) 2025 Axel Covacich. All rights reserved.
 * @project Operating Systems II - FCEFYN UNC
 */
#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdio.h>
#include <sys/socket.h>

// --- Generic Transport Function Types ---

/**
 * @brief A function pointer type for a generic receive function.
 */
typedef ssize_t (*recv_fn)(int sockfd, void *buf, size_t len, int flags);

/**
 * @brief A function pointer type for a generic send function.
 */
typedef ssize_t (*send_fn)(int sockfd, const void *buf, size_t len, int flags);

// --- TCP Wrapper Functions ---

/**
 * @brief A wrapper for the standard read() call, matching the recv_fn signature.
 */
ssize_t tcp_recv(int sockfd, void *buf, size_t len, int flags);

/**
 * @brief A wrapper for the standard write() call, matching the send_fn signature.
 */
ssize_t tcp_send(int sockfd, const void *buf, size_t len, int flags);

// --- UDP Wrapper Functions ---

/// @brief This is the setter for the adress to use in udp protocol
/// @param addr server adress
/// @param len length of server adress
void initialize_udp_peer_address(const struct sockaddr *addr, socklen_t len);

/**
 * @brief A wrapper for the standard recvfrom() call, matching the recv_fn signature.
 */
ssize_t udp_recv(int sockfd, void *buf, size_t len, int flags);

/**
 * @brief A wrapper for the standard sendto() call, matching the send_fn signature.
 */
ssize_t udp_send(int sockfd, const void *buf, size_t len, int flags);

// --- Testing Utilities ---

/**
 * @brief (For testing only) Retrieves a pointer to the internal static peer address.
 */
const struct sockaddr_storage *get_peer_address_for_test(void);

#endif // TRANSPORT_H