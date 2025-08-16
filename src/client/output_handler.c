// output_handler.c

#include "output_handler.h"
#include "cJSON.h"
#include "client.h"
#include "logger.h"
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
        logger_log("Output_handler", WARNING, "Error: Could not parse server response.");
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
        logger_log("Output_handler", INFO, "Inventory report retrieved successfully.");
    } else {

        cJSON *message = cJSON_GetObjectItem(root, "message");
        if (message && cJSON_IsString(message)) {
            logger_log("Output_handler", WARNING,
                       ("Error while processing inventory report from server: %s", message->valuestring));
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
        logger_log("Output_handler", INFO, "Stock report retrieved successfully.");
    } else {

        cJSON *message = cJSON_GetObjectItem(root, "message");
        if (message && cJSON_IsString(message)) {
            logger_log("Output_handler", WARNING,
                       ("Error while processing stock report from server: %s", message->valuestring));
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
            logger_log("Output_handler", INFO, "Inventory history retrieved successfully.");
        } else {
            logger_log("Output_handler", WARNING, "No inventory history found for this client.");
            fprintf(output_stream, "No inventory history found for this client.\n"); // NOLINT
        }

    } else {

        cJSON *message = cJSON_GetObjectItem(root, "message");
        if (message && cJSON_IsString(message)) {
            logger_log("Output_handler", WARNING,
                       ("Error while retrieving inventory history from server: %s", message->valuestring));
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
        logger_log("Output_handler", ERROR, "Error: Invalid server response (missing status).");
        fprintf(output_stream, "Error: Invalid server response (missing status).\n"); // NOLINT
        cJSON_Delete(root);
        return;
    }

    cJSON *message = cJSON_GetObjectItem(root, "message");
    if (!message || !cJSON_IsString(message)) {
        logger_log("Output_handler", ERROR, "Error: Invalid server response (missing message).");
        fprintf(output_stream, "Error: Invalid server response (missing message).\n"); // NOLINT
        cJSON_Delete(root);
        return;
    }

    if (strcmp(status->valuestring, "success") == 0) {

        cJSON *client_id = cJSON_GetObjectItem(root, "client_id");
        if (!client_id || !cJSON_IsString(client_id)) {
            logger_log("Output_handler", ERROR, "Error: Login response missing client_id.");
            fprintf(output_stream, "Error: Login response missing client_id.\n"); // NOLINT
            cJSON_Delete(root);
            return;
        }
        fprintf(output_stream, "-Status: %s\n", status->valuestring);   // NOLINT
        fprintf(output_stream, "-Message: %s\n", message->valuestring); // NOLINT
        logger_log("Output_handler", INFO, ("Login successful. Client ID: %s", client_id->valuestring));

        client_context_set_id(context, client_id->valuestring);

        if (!session_start_aux_threads(context)) {
            logger_log("Output_handler", ERROR, "Error: Failed to start auxiliary threads.");
            fprintf(output_stream, "Error: Failed to start auxiliary threads.\n"); // NOLINT
        }
        if (!launch_dashboard(context)) {
            logger_log("Output_handler", ERROR, "Error: Failed to launch dashboard.");
            fprintf(output_stream, "Error: Failed to launch dashboard.\n"); // NOLINT
        }

    } else {
        logger_log("Output_handler", WARNING, ("Login failed: %s", message->valuestring));
        fprintf(output_stream, "Error from server: %s\n", message->valuestring); // NOLINT
    }

    cJSON_Delete(root);
}
