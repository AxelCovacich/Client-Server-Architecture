/**
 * @file output_handler.h
 * @brief Declares functions responsible for formatting and printing server responses to the console.
 * @author Axel Covacich
 * @copyright Copyright (c) 2025 Axel Covacich. All rights reserved.
 * @project Operating Systems II - FCEFYN UNC
 */

#ifndef OUTPUT_HANDLER_H
#define OUTPUT_HANDLER_H

#include "client_context.h"
#include <stdio.h>

/**
 * @brief Main dispatcher for printing server responses in a human-readable format.
 *
 * This function inspects the original user command and routes the server's JSON
 * response to the appropriate specialized print function.
 * @param server_response The raw JSON string received from the server.
 * @param command The original command string entered by the user.
 * @param output_stream The file stream to write the formatted output to (e.g., stdout).
 */
void print_readable_response(ClientContext *context, const char *server_response, const char *input_buffer,
                             FILE *output_stream);

/**
 * @brief Formats and prints a full inventory report from a JSON response.
 * @param response_string The raw JSON string containing the inventory data.
 * @param output_stream The file stream to write the formatted output to.
 */
void print_inventory_response(const char *response_string, FILE *output_stream);

/**
 * @brief Formats and prints a single stock item report from a JSON response.
 * @param response_string The raw JSON string containing the stock data.
 * @param output_stream The file stream to write the formatted output to.
 */
void print_get_stock_response(const char *response_string, FILE *output_stream);

/**
 * @brief Formats and prints a client's transaction history from a JSON response.
 * @param response_string The raw JSON string containing the history data array.
 * @param output_stream The file stream to write the formatted output to.
 */
void print_get_history_response(const char *response_string, FILE *output_stream);

void handle_login_response(ClientContext *context, const char *response_string, FILE *output_stream);

#endif // OUTPUT_HANDLER_H
