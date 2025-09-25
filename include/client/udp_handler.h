#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

#include "client_context.h"
#include "transport.h"

#define BUFFER_SIZE_UDP 256 // Define a buffer size for UDP messages
#define SLEEP_KEEPALIVE_TIME 60
#define KEEPALIVE_MSG_SIZE 128

// Function for the thread that listens for UDP messages
void *udp_listener_thread_func(void *arg);

/**
 * @brief The main function for the background keepalive thread.
 * This function runs in a continuous loop, sending a UDP keepalive datagram
 * to the server every 60 seconds.
 * @param arg A pointer to an integer containing the client context with the udp socket, clientId and more client info.
 * @return Always returns NULL.
 */
void *keepalive_thread_func(void *arg);

#endif