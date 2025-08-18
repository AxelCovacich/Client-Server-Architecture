#include "client_context.h"
#include "logger.h"
#include "output_handler.h"
#include "unity.h"
#include <stdio.h>

/**
 * @brief Tests that print_simple_response correctly formats a success message.
 */
void test_print_simple_response_formats_success() {

    const char *server_json = "{\"status\":\"success\",\"message\":\"STATUS: OK\"}";

    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    print_readable_response(&context, server_json, "status", memory_stream);
    fclose(memory_stream);

    const char *expected_output = "-Status: success\n-Message: STATUS: OK\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_print_get_inventory_response_formats_success() {

    const char *server_json_response =
        "{\"status\":\"success\",\"message\":\"Inventory data "
        "retrieved.\",\"data\":{\"food\":{\"water\":100},\"medicine\":{\"bandages\":500}}}";
    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    print_readable_response(&context, server_json_response, "get_inventory", memory_stream);
    fclose(memory_stream);

    const char *expected_output = "--- Inventory Report ---\n"
                                  "  Category: food\n"
                                  "    - water: 100\n"
                                  "  Category: medicine\n"
                                  "    - bandages: 500\n"
                                  "------------------------\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_print_get_inventory_response_formats_error() {

    const char *server_json_response =
        "{\"status\":\"error\",\"message\":\"Inventory data for client warehouse-1 is empty.\"}";
    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    print_readable_response(&context, server_json_response, "get_inventory", memory_stream);
    fclose(memory_stream);

    const char *expected_output = "Error from server: Inventory data for client warehouse-1 is empty.\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_print_get_history_response_formats_success() {

    const char *server_json_response =
        "{\"status\":\"success\",\"message\":\"Inventory history "
        "retrieved.\",\"data\":[{\"component\":\"Inventory\",\"level\":\"INFO\",\"message\":\"Stock updated for "
        "food.meat to "
        "100\",\"timestamp\":\"2025-08-18 20:15:36 "
        "UTC\"},{\"component\":\"Inventory\",\"level\":\"INFO\",\"message\":\"Stock updated "
        "for food.water to 100\",\"timestamp\":\"2025-08-18 20:10:36 UTC\"}]}";
    char output_buffer[1024] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    // const char *log_path = "../../var/logs/test_client.log";
    // logger_init(log_path);
    print_readable_response(&context, server_json_response, "get_history", memory_stream);
    fclose(memory_stream);

    const char *expected_output =
        "--- Inventory History ---\n  - [2025-08-18 20:15:36 UTC] Stock updated for food.meat to 100\n  - "
        "[2025-08-18 20:10:36 UTC] Stock updated for food.water to 100\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_print_get_history_response_formats_empty() {

    const char *server_json_response = "{\"status\":\"success\",\"message\":\"Inventory history retrieved.\"}";
    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    print_readable_response(&context, server_json_response, "get_history", memory_stream);
    fclose(memory_stream);

    const char *expected_output = "No inventory history found for this client.\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_print_get_history_response_error() {

    const char *server_json_response =
        "{\"status\":\"error\",\"message\":\"An internal server error occurred. Please reconnect.\"}";
    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    print_readable_response(&context, server_json_response, "get_history", memory_stream);
    fclose(memory_stream);

    const char *expected_output = "Error from server: An internal server error occurred. Please reconnect.\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_print_get_stock_response_formats_success() {

    const char *server_json_response =
        "{\"status\":\"success\",\"message\":\"Stock data retrieved "
        "successfully.\",\"data\":{\"category\":\"food\",\"item\":\"meat\",\"quantity\":300}}";
    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    print_readable_response(&context, server_json_response, "get_stock", memory_stream);
    fclose(memory_stream);

    const char *expected_output =
        "--- Stock Report ---\n  - Item: meat\n  - Category: food\n  - Quantity: 300\n------------------------\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_print_get_stock_response_formats_error() {

    const char *server_json_response =
        "{\"status\":\"error\",\"message\":\"Item not found for the specified client/category.\"}";
    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    print_readable_response(&context, server_json_response, "get_stock", memory_stream);
    fclose(memory_stream);

    const char *expected_output = "Error from server: Item not found for the specified client/category.\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_login_success() {

    const char *server_json = "{\"status\":\"success\",\"message\":\"Login successful! Welcome warehouse-1.\", "
                              "\"client_id\":\"warehouse-1\"}";

    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    handle_login_response(&context, server_json, memory_stream);
    fclose(memory_stream);

    const char *expected_output = "-Status: success\n-Message: Login successful! Welcome warehouse-1.\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_login_missing_client_id() {

    const char *server_json = "{\"status\":\"success\",\"message\":\"Login successful! Welcome warehouse-1.\"}";

    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    handle_login_response(&context, server_json, memory_stream);
    fclose(memory_stream);

    const char *expected_output = "Error: Login response missing client_id.\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}

void test_login_error_response() {

    const char *server_json = "{\"status\":\"error\",\"message\":\"Login failed. Invalid hostname or password.\"}";

    char output_buffer[256] = {0};

    FILE *memory_stream = fmemopen(output_buffer, sizeof(output_buffer), "w");
    TEST_ASSERT_NOT_NULL(memory_stream);

    ClientContext context;
    client_context_init(&context);
    handle_login_response(&context, server_json, memory_stream);
    fclose(memory_stream);

    const char *expected_output = "Error from server: Login failed. Invalid hostname or password.\n";
    TEST_ASSERT_EQUAL_STRING(expected_output, output_buffer);
}