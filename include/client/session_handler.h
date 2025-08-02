/**
 * @file session_handler.h
 * @brief Manages the client's interactive session and background tasks.
 *
 * This module contains the main communication loop for the TCP session and the
 * logic for the asynchronous UDP keepalive thread. It orchestrates the interaction
 * between the user, the network transport layer, and the command processing logic.
 * @author Axel Covacich
 * @copyright Copyright (c) 2025 Axel Covacich. All rights reserved.
 * @project Operating Systems II - FCEFYN UNC
 */

#ifndef SESSION_HANDLER_H
#define SESSION_HANDLER_H

#include "transport.h"

#define SLEEP_KEEPALIVE_TIME 60

/**
 * @brief Starts the main interactive communication loop with the server.
 *
 * This function handles the entire lifecycle of a client's session after a
 * successful connection and login. It reads user input, dispatches actions,
 * and processes server responses.
 * @param sockfd The active TCP socket file descriptor.
 * @param recieve A function pointer to the receive function to use (e.g., tcp_recv).
 * @param send A function pointer to the send function to use (e.g., tcp_send).
 * @return 0 on a clean shutdown, or -1 if a critical error occurs.
 */
int start_communication(int sockfd, recv_fn recieve, send_fn send);

/**
 * @brief The main function for the background keepalive thread.
 *
 * This function runs in a continuous loop, sending a UDP keepalive datagram
 * to the server every 60 seconds.
 * @param arg A pointer to an integer containing the UDP socket file descriptor.
 * @return Always returns NULL.
 */
void *keepalive_thread_func(void *arg);

#endif // SESSION_HANDLER_H