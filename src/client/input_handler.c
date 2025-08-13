#include "input_handler.h"
#include "cJSON.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

json_build_result build_json_from_input(char *raw_input) {

    char *command = strtok(raw_input, " ");

    // --- Logic of parsing arguments ---
    if (strcmp(command, "update_stock") == 0) {

        return build_json_for_update_stock(command);
    }

    if (strcmp(command, "get_stock") == 0) {

        return build_json_for_get_stock(command);
    }

    if (strcmp(command, "login") == 0) {

        return build_json_for_login(command);
    }

    // get_history, status, get_inventory fall here because its only one command to send
    return build_json_for_single_command(command);
}

UserInputAction process_user_input(char *buffer) {

    buffer[strcspn(buffer, "\n")] = 0;

    if (strcmp("end", buffer) == 0) {
        return INPUT_ACTION_QUIT;
    }
    if (strlen(buffer) == 0) {
        return INPUT_ACTION_CONTINUE;
    }
    return INPUT_ACTION_SEND;
}

bool parse_arguments(int argc, const char *argv[], client_config *out_config) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage: %s <host> <tcp_port>(optional) <udp_port>(optional)\n", argv[0]); // NOLINT
        return false;
    }

    // 1. Set defaults
    out_config->port_tcp = DEFAULT_TCP_PORT;
    out_config->port_udp = DEFAULT_UDP_PORT;

    // 2. Override with env vars if present
    const char *env_tcp_port = getenv("CLIENT_TCP_PORT");
    if (env_tcp_port != NULL) {
        int tcp_port_num = atoi(env_tcp_port);
        if (tcp_port_num <= 0 || tcp_port_num > MAX_PORT_NUMBER) {
            fprintf(stderr, "Error: TCP port in environment is invalid.\n");
            return false;
        }
        out_config->port_tcp = env_tcp_port;
    }

    const char *env_udp_port = getenv("CLIENT_UDP_PORT");
    if (env_udp_port != NULL) {
        int udp_port_num = atoi(env_udp_port);
        if (udp_port_num <= 0 || udp_port_num > MAX_PORT_NUMBER) {
            fprintf(stderr, "Error: UDP port in environment is invalid.\n");
            return false;
        }
        out_config->port_udp = env_udp_port;
    }

    // 3. Override with arguments if present
    if (argc >= 3) {
        int tcp_port_num = atoi(argv[2]);
        if (tcp_port_num <= 0 || tcp_port_num > MAX_PORT_NUMBER) {
            fprintf(stderr, "Error: TCP port must be a number between 1 and 65535.\n");
            return false;
        }
        out_config->port_tcp = argv[2];
    }
    if (argc == 4) {
        int udp_port_num = atoi(argv[3]);
        if (udp_port_num <= 0 || udp_port_num > MAX_PORT_NUMBER) {
            fprintf(stderr, "Error: UDP port must be a number between 1 and 65535.\n");
            return false;
        }
        out_config->port_udp = argv[3];
    }

    out_config->host = argv[1];
    return true;
}

json_build_result build_json_for_get_stock(char *command) {

    // obtainging arguments
    char *category = strtok(NULL, " ");
    char *item = strtok(NULL, " ");

    if (item == NULL || category == NULL) {
        fprintf(stderr, "Error: get_stock requires <category> and <item>.\n"); // NOLINT
        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_SYNTAX};
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        printf("Error creating the cJSON object...\n");
        logger_log("Input_handler", ERROR, "Error creating the cJSON object for get_stock.");
        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_MEMORY};
    }

    // building the json objects with the arguments obtained
    cJSON_AddStringToObject(root, "command", command);

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "category", category);
    cJSON_AddStringToObject(payload, "item", item);
    cJSON_AddItemToObject(root, "payload", payload);

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return (json_build_result){.json_string = json_string, .status = JSON_BUILD_SUCCESS};
}

json_build_result build_json_for_update_stock(char *command) {

    // obtainging arguments
    char *category = strtok(NULL, " ");
    char *item = strtok(NULL, " ");
    char *quantity_str = strtok(NULL, " ");

    if (item == NULL || quantity_str == NULL || category == NULL) {
        fprintf(stderr, "Error: update_stock requires <category>, <item> and <quantity>.\n"); // NOLINT
        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_SYNTAX};
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        printf("Error creating the cJSON object...\n");
        logger_log("Input_handler", ERROR, "Error creating the cJSON object for update_stock.");

        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_MEMORY};
    }

    char *endptr = NULL;
    long quantity = strtol(quantity_str, &endptr, BASE);

    if (*endptr != '\0' || endptr == quantity_str) {

        fprintf(stderr, "Error: quantity must be a valid number.\n"); // NOLINT
        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_SYNTAX};
    }

    // building the json objects with the arguments obtained
    cJSON_AddStringToObject(root, "command", command);

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "category", category);
    cJSON_AddStringToObject(payload, "item", item);
    cJSON_AddNumberToObject(payload, "quantity", (double)quantity);
    cJSON_AddItemToObject(root, "payload", payload);

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return (json_build_result){.json_string = json_string, .status = JSON_BUILD_SUCCESS};
}

json_build_result build_json_for_login(char *command) {

    char *username = strtok(NULL, " ");
    char *password = strtok(NULL, " ");

    if (username == NULL || password == NULL) {
        fprintf(stderr, "Error: login requires <username> and <password>.\n"); // NOLINT
        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_SYNTAX};
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        printf("Error creating the cJSON object...\n");
        logger_log("Input_handler", ERROR, "Error creating the cJSON object for login.");
        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_MEMORY};
    }

    // building the json objects with the arguments obtained
    cJSON_AddStringToObject(root, "command", command);

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "hostname", username);
    cJSON_AddStringToObject(payload, "password", password);
    cJSON_AddItemToObject(root, "payload", payload);
    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return (json_build_result){.json_string = json_string, .status = JSON_BUILD_SUCCESS};
}

json_build_result build_json_for_single_command(char *command) {

    cJSON *root = cJSON_CreateObject();

    if (root == NULL) {
        printf("Error creating the cJSON object...\n");
        logger_log("Input_handler", ERROR, "Error creating the cJSON object for single command.");
        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_MEMORY};
    }
    // for simple commands like status
    cJSON_AddStringToObject(root, "command", command);
    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return (json_build_result){.json_string = json_string, .status = JSON_BUILD_SUCCESS};
}
