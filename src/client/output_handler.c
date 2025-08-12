// output_handler.c

#include "output_handler.h"
#include "cJSON.h"
#include "client.h"
#include "session_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_readable_response(ClientContext *context, const char *server_response, const char *input_buffer,
                             FILE *output_stream) {

    char input_buffer_copy[BUFFER_SIZE];
    strncpy(input_buffer_copy, input_buffer, sizeof(input_buffer_copy) - 1); // NOLINT
    input_buffer_copy[sizeof(input_buffer_copy) - 1] = '\0';

    char *command = strtok(input_buffer_copy, " ");

    if (strcmp(command, "get_inventory") == 0) {

        print_inventory_response(server_response, output_stream);
        return;
    }

    if (strcmp(command, "get_stock") == 0) {

        print_get_stock_response(server_response, output_stream);
        return;
    }

    if (strcmp(command, "get_history") == 0) {

        print_get_history_response(server_response, output_stream);
        return;
    }

    if (strcmp(command, "login") == 0) {

        handle_login_response(context, server_response, output_stream);
        return;
    }

    cJSON *root = cJSON_Parse(server_response);
    if (root == NULL) {
        fprintf(output_stream, "Error: Could not parse server response.\n"); // NOLINT
        return;
    }
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (status && cJSON_IsString(status)) {
        fprintf(output_stream, "-Status: %s\n", status->valuestring); // NOLINT
    }
    cJSON *message = cJSON_GetObjectItem(root, "message");
    if (message && cJSON_IsString(message)) {
        fprintf(output_stream, "-Message: %s\n", message->valuestring); // NOLINT
    }

    cJSON_Delete(root);
}

void print_inventory_response(const char *response_string, FILE *output_stream) {

    cJSON *root = cJSON_Parse(response_string);
    if (root == NULL) {
        fprintf(output_stream, "Error: Could not parse server response.\n"); // NOLINT
        return;
    }

    cJSON *status = cJSON_GetObjectItem(root, "status");
    if ((strcmp(status->valuestring, "success")) == 0) {
        fprintf(output_stream, "--- Inventory Report ---\n"); // NOLINT

        cJSON *data = cJSON_GetObjectItem(root, "data");
        if (data) {
            cJSON *category = NULL;

            cJSON_ArrayForEach(category, data) {
                fprintf(output_stream, "  Category: %s\n", category->string); // NOLINT

                cJSON *item = NULL;

                cJSON_ArrayForEach(item, category) {
                    fprintf(output_stream, "    - %s: %d\n", item->string, item->valueint); // NOLINT
                }
            }
        }
        fprintf(output_stream, "------------------------\n"); // NOLINT
    } else {

        cJSON *message = cJSON_GetObjectItem(root, "message");
        if (message && cJSON_IsString(message)) {
            fprintf(output_stream, "Error from server: %s\n", message->valuestring); // NOLINT
        }
    }

    cJSON_Delete(root);
}

void print_get_stock_response(const char *response_string, FILE *output_stream) {

    cJSON *root = cJSON_Parse(response_string);
    if (root == NULL) {
        fprintf(output_stream, "Error: Could not parse server response.\n"); // NOLINT
        return;
    }

    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (strcmp(status->valuestring, "success") == 0) {
        fprintf(output_stream, "--- Stock Report ---\n"); // NOLINT

        cJSON *data = cJSON_GetObjectItem(root, "data");
        if (data) {
            cJSON *category = cJSON_GetObjectItem(data, "category");
            cJSON *item = cJSON_GetObjectItem(data, "item");
            cJSON *quantity = cJSON_GetObjectItem(data, "quantity");

            if (category && item && quantity) {
                fprintf(output_stream, "  - Item: %s\n", item->valuestring);         // NOLINT
                fprintf(output_stream, "  - Category: %s\n", category->valuestring); // NOLINT
                fprintf(output_stream, "  - Quantity: %d\n", quantity->valueint);    // NOLINT
            }
        }
        fprintf(output_stream, "------------------------\n"); // NOLINT
    } else {

        cJSON *message = cJSON_GetObjectItem(root, "message");
        if (message && cJSON_IsString(message)) {
            fprintf(output_stream, "Error from server: %s\n", message->valuestring); // NOLINT
        }
    }

    cJSON_Delete(root);
}

void print_get_history_response(const char *response_string, FILE *output_stream) {

    cJSON *root = cJSON_Parse(response_string);
    if (root == NULL) {
        fprintf(output_stream, "Error: Could not parse server response.\n"); // NOLINT
        return;
    }

    cJSON *status = cJSON_GetObjectItem(root, "status");
    if ((strcmp(status->valuestring, "success")) == 0) {

        cJSON *data_array = cJSON_GetObjectItem(root, "data");

        if (data_array && cJSON_IsArray(data_array) && cJSON_GetArraySize(data_array) > 0) {
            cJSON *log_entry = NULL;

            fprintf(output_stream, "--- Inventory History ---\n"); // NOLINT
            cJSON_ArrayForEach(log_entry, data_array) {
                cJSON *timestamp = cJSON_GetObjectItem(log_entry, "timestamp");
                cJSON *message = cJSON_GetObjectItem(log_entry, "message");

                if (timestamp && message) {
                    fprintf(output_stream, "  - [%d] %s\n", timestamp->valueint, message->valuestring); // NOLINT
                }
            }
        } else {
            fprintf(output_stream, "No inventory history found for this client.\n"); // NOLINT
        }

    } else {

        cJSON *message = cJSON_GetObjectItem(root, "message");
        if (message && cJSON_IsString(message)) {
            fprintf(output_stream, "Error from server: %s\n", message->valuestring); // NOLINT
        }
    }

    cJSON_Delete(root);
}

void handle_login_response(ClientContext *context, const char *response_string, FILE *output_stream) {
    cJSON *root = cJSON_Parse(response_string);
    if (root == NULL) {
        fprintf(output_stream, "Error: Could not parse server response.\n"); // NOLINT
        return;
    }

    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (!status || !cJSON_IsString(status)) {
        fprintf(output_stream, "Error: Invalid server response (missing status).\n"); // NOLINT
        cJSON_Delete(root);
        return;
    }

    cJSON *message = cJSON_GetObjectItem(root, "message");
    if (!message || !cJSON_IsString(message)) {
        fprintf(output_stream, "Error: Invalid server response (missing message).\n"); // NOLINT
        cJSON_Delete(root);
        return;
    }

    if (strcmp(status->valuestring, "success") == 0) {

        cJSON *client_id = cJSON_GetObjectItem(root, "client_id");
        if (!client_id || !cJSON_IsString(client_id)) {
            fprintf(output_stream, "Error: Login response missing client_id.\n"); // NOLINT
            cJSON_Delete(root);
            return;
        }
        fprintf(output_stream, "-Status: %s\n", status->valuestring);   // NOLINT
        fprintf(output_stream, "-Message: %s\n", message->valuestring); // NOLINT

        client_context_set_id(context, client_id->valuestring);

        session_start_aux_threads(context);
    } else {
        fprintf(output_stream, "Error from server: %s\n", message->valuestring);
    }

    cJSON_Delete(root);
}
