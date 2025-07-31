#include "input_handler.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

json_build_result build_json_from_input(char *raw_input) {

    char *command = strtok(raw_input, " ");

    // --- Logic of parsing arguments ---
    if (strcmp(command, "update_stock") == 0) {
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
            return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_MEMORY};
        }

        // building the json objects with the arguments obtained
        cJSON_AddStringToObject(root, "command", command);

        cJSON *payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "category", category);
        cJSON_AddStringToObject(payload, "item", item);
        cJSON_AddNumberToObject(payload, "quantity", atoi(quantity_str));
        cJSON_AddItemToObject(root, "payload", payload);

        char *json_string = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        return (json_build_result){.json_string = json_string, .status = JSON_BUILD_SUCCESS};
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL) {
        printf("Error creating the cJSON object...\n");
        return (json_build_result){.json_string = NULL, .status = JSON_BUILD_ERROR_MEMORY};
    }
    // for simple commands like status
    cJSON_AddStringToObject(root, "command", command);
    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return (json_build_result){.json_string = json_string, .status = JSON_BUILD_SUCCESS};
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
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]); // NOLINT
        return false;
    }

    int port_num = atoi(argv[2]);
    if (port_num <= 0 || port_num > 65535) {
        fprintf(stderr, "Error: Port must be a number between 1 and 65535.\n"); // NOLINT
        return false;
    }

    out_config->host = argv[1];
    out_config->port_tcp = argv[2];

    snprintf(out_config->port_udp, sizeof(out_config->port_udp), "%d", port_num + 1);

    return true;
}