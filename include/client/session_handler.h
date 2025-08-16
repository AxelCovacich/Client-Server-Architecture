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

#include "client.h"
#include "client_context.h"
#include "input_handler.h"
#include "transport.h"
#include <stdbool.h>

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
int start_communication(ClientContext *context, recv_fn recieve, send_fn send);

/**
 * @brief Starts the auxiliary threads for the session (keepalive and udp listener).
 * @param context Pointer to the client context.
 */
bool session_start_aux_threads(ClientContext *context);

/**
 * @brief Executes a specific client action based on parsed user input.
 *
 * This function acts as a dispatcher for the main communication loop. It takes
 * a UserInputAction and performs the corresponding task, such as building and
 * sending a JSON message or handling the quit sequence.
 *
 * @param sockfd The active socket file descriptor for the server connection.
 * @param action The UserInputAction enum value determining which action to perform.
 * @param buffer The raw user input buffer, used to build the JSON message for SEND actions.
 * @return A TransactionResult enum value indicating the outcome of the operation.
 */
transaction_result execute_client_action(ClientContext *context, UserInputAction action, char *buffer, recv_fn recieve,
                                         send_fn send);

bool launch_dashboard(ClientContext *context);

void signal_handler(int signum);

#endif // SESSION_HANDLER_H