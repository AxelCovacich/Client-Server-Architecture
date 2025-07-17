#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <stddef.h>

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

#endif // INPUT_HANDLER_H