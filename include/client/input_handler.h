/**
 * @file input_handler.h
 * @brief Handles parsing of user command-line input and building of JSON messages.
 *
 * This module is responsible for interpreting the raw strings entered by the user,
 * validating their syntax, and constructing the appropriate JSON payload to be

 * sent to the server.
 * @author Axel Covacich
 * @copyright Copyright (c) 2025 Axel Covacich. All rights reserved.
 * @project Operating Systems II - FCEFYN UNC
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_PORT_NUMBER 65535
#define DEFAULT_PORT "8888" // Default port
#define BASE 10

/**
 * @brief Defines the set of possible actions for the client's main loop.
 *
 * This enumeration is returned by the input processing function to tell the
 * main communication loop what to do next. It allows for a clean, state-based
 * control flow.
 */
typedef enum {
    INPUT_ACTION_SEND,
    INPUT_ACTION_QUIT,
    INPUT_ACTION_CONTINUE,
    INPUT_ACTION_ERROR,
} UserInputAction;

/**
 * @brief Processes the user's raw input and determines the next action.
 * @param buffer The buffer containing input from fgets.
 * @return A UserInputAction enum value indicating what the main loop should do.
 */
UserInputAction process_user_input(char *buffer);

/**
 * @brief Defines the enum of posible outcomes for the json building execution,
 * `JSON_BUILD_SUCCESS` on a successfull operation, `JSON_BUILD_ERROR_SYNTAX` if an syntaxis error was found in order
 * to try again, `JSON_BUILD_ERROR_MEMORY` if a critical error at building was found, should exit
 */
typedef enum {
    JSON_BUILD_SUCCESS,
    JSON_BUILD_ERROR_SYNTAX, // Syntaxis error, try again
    JSON_BUILD_ERROR_MEMORY  // Fatal error, exit
} json_build_status;

/**
 * @brief Defines the struct for the result of building the json object
 * `json_string` will contain the string version of the json object created or NULL on error.
 * `status` will contain the status of the json building operation:
 * `JSON_BUILD_SUCCESS`,`JSON_BUILD_ERROR_SYNTAX`,`SON_BUILD_ERROR_MEMORY`
 */
typedef struct {
    char *json_string; // NULL if error
    json_build_status status;
} json_build_result;

/**
 * @brief Parses user input and builds a JSON string for the server.
 * @param raw_input The raw string from the user.
 * @return A dynamically allocated string containing the JSON message. The caller
 * MUST free this string. Returns NULL on error or for non-sending actions.
 */
json_build_result build_json_from_input(char *raw_input);

typedef struct {
    const char *host;
    const char *port_tcp;
    const char *port_udp;
} client_config;

/**
 * @brief Parses command-line arguments for the client.
 * @param argc The argument count from main.
 * @param argv The argument vector from main.
 * @param out_config A pointer to a ClientConfig struct to be filled.
 * @return True on successful parsing, false otherwise.
 */
bool parse_arguments(int argc, const char *argv[], client_config *out_config);

/**
 * @brief Builds a JSON message for an 'update_stock' command.
 * @param command The full command string from the user (e.g., "update_stock food meat 100").
 * @return A json_build_result struct containing the JSON string and a status code.
 */
json_build_result build_json_for_update_stock(char *command);

/**
 * @brief Builds a JSON message for a 'get_stock' command.
 * @param command The full command string from the user (e.g., "get_stock food meat").
 * @return A json_build_result struct containing the JSON string and a status code.
 */
json_build_result build_json_for_get_stock(char *command);

/**
 * @brief Builds a JSON message for a 'login' command.
 * @param command The full command string from the user (e.g., "login user pass").
 * @return A json_build_result struct containing the JSON string and a status code.
 */
json_build_result build_json_for_login(char *command);

/**
 * @brief Builds a JSON message for a simple, single-word command (e.g., "status", "get_history").
 * @param command The single-word command string from the user.
 * @return A json_build_result struct containing the JSON string and a status code.
 */
json_build_result build_json_for_single_command(char *command);

/**
 * @brief Builds a JSON message for a 'register_user' command.
 * @param command The full command string from the user (e.g., "register_user new_user new_pass").
 * @return A json_build_result struct containing the JSON string and a status code.
 */
json_build_result build_json_for_register_user(char *command);

/**
 * @brief Configures TCP and UDP ports from command-line arguments.
 * @param argc The argument count from main.
 * @param argv The argument vector from main.
 * @param out_config A pointer to a ClientConfig struct to be filled with port info.
 * @return True on successful parsing, false otherwise.
 */
bool config_arguments_ports(int argc, const char *argv[], client_config *out_config);

#endif // INPUT_HANDLER_H
