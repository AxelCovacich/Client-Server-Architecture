
#include "client.h"
#include "input_handler.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Tests that a normal string message results in the SEND action.
 */
void test_process_input_returns_send_for_normal_message() {

    char buffer[] = "hello server\n";

    UserInputAction result = process_user_input(buffer);

    TEST_ASSERT_EQUAL_INT(INPUT_ACTION_SEND, result);
}

/**
 * @brief Tests that the "end" command results in the QUIT action.
 */
void test_process_input_returns_quit_for_end_command() {
    char buffer[] = "end\n";

    UserInputAction result = process_user_input(buffer);

    TEST_ASSERT_EQUAL_INT(INPUT_ACTION_QUIT, result);
}

/**
 * @brief Tests that an empty input (just a newline) results in the CONTINUE action.
 */
void test_process_input_returns_continue_for_empty_input() {
    char buffer[] = "\n";

    UserInputAction result = process_user_input(buffer);

    TEST_ASSERT_EQUAL_STRING("", buffer);
    TEST_ASSERT_EQUAL_INT(INPUT_ACTION_CONTINUE, result);
}

/**
 * @brief Tests that a simple, single-word input is correctly formatted into a JSON command.
 */
void test_build_json_for_simple_command() {

    char input[] = "Status";

    const char *expected_json = "{\"command\":\"Status\"}";

    json_build_result result_json = build_json_from_input(input);

    TEST_ASSERT_EQUAL_INT(JSON_BUILD_SUCCESS, result_json.status);
    TEST_ASSERT_NOT_NULL(result_json.json_string);
    TEST_ASSERT_EQUAL_STRING(expected_json, result_json.json_string);

    free(result_json.json_string);
}

/**
 * @brief Tests that a command with arguments is correctly formatted with a payload.
 */
void test_build_json_for_command_with_args() {
    char input[] = "update_stock medicine medkit 150";
    // The order is not garantized but 99.9% it is
    const char *expected_json =
        "{\"command\":\"update_stock\",\"payload\":{\"category\":\"medicine\",\"item\":\"medkit\",\"quantity\":150}}";

    json_build_result result = build_json_from_input(input);

    TEST_ASSERT_EQUAL_INT(JSON_BUILD_SUCCESS, result.status);
    TEST_ASSERT_NOT_NULL(result.json_string);
    TEST_ASSERT_EQUAL_STRING(expected_json, result.json_string);

    free(result.json_string);
}

/**
 * @brief Tests that the function returns a syntax error for incomplete commands.
 */
void test_build_json_fails_on_missing_arguments() {
    char input[] = "update_stock medkit";

    json_build_result result = build_json_from_input(input);

    TEST_ASSERT_EQUAL_INT(JSON_BUILD_ERROR_SYNTAX, result.status);
    TEST_ASSERT_NULL(result.json_string);
}

void test_input_arguments_success() {
    client_config config;
    const char *argv[] = {"./client", "localhost", "8000", "8001"};
    int argc = 4;

    TEST_ASSERT_TRUE(parse_arguments(argc, argv, &config));
    TEST_ASSERT_EQUAL_STRING("localhost", config.host);
    TEST_ASSERT_EQUAL_STRING("8000", config.port_tcp);
    TEST_ASSERT_EQUAL_STRING("8001", config.port_udp);
}

void test_parse_arguments_fails_on_insufficient_args() {

    const char *argv[] = {"./client"};
    int argc = 1;
    client_config config;

    bool result = parse_arguments(argc, argv, &config);

    TEST_ASSERT_FALSE(result);
}

/**
 * @brief Tests that argument parsing fails with an invalid port number.
 */
void test_parse_arguments_fails_on_invalid_port() {

    const char *argv[] = {"./client", "localhost", "not_a_port"};
    int argc = 3;
    client_config config;

    bool result = parse_arguments(argc, argv, &config);

    TEST_ASSERT_FALSE(result);
}

void test_build_json_get_stock_success() {
    char input[] = "get_stock medicine bandages";
    // The order is not garantized but 99.9% it is
    const char *expected_json =
        "{\"command\":\"get_stock\",\"payload\":{\"category\":\"medicine\",\"item\":\"bandages\"}}";

    json_build_result result = build_json_from_input(input);

    TEST_ASSERT_EQUAL_INT(JSON_BUILD_SUCCESS, result.status);
    TEST_ASSERT_NOT_NULL(result.json_string);
    TEST_ASSERT_EQUAL_STRING(expected_json, result.json_string);

    free(result.json_string);
}

void test_build_json_get_stock_fails_on_missing_arguments() {
    char input[] = "get_stock food";

    json_build_result result = build_json_from_input(input);

    TEST_ASSERT_EQUAL_INT(JSON_BUILD_ERROR_SYNTAX, result.status);
    TEST_ASSERT_NULL(result.json_string);
}

void test_build_json_login_success() {
    char input[] = "login username password";

    const char *expected_json =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"username\",\"password\":\"password\"}}";

    json_build_result result = build_json_from_input(input);

    TEST_ASSERT_EQUAL_INT(JSON_BUILD_SUCCESS, result.status);
    TEST_ASSERT_NOT_NULL(result.json_string);
    TEST_ASSERT_EQUAL_STRING(expected_json, result.json_string);

    free(result.json_string);
}

void test_build_json_login_missing_field() {
    char input[] = "login  password";

    json_build_result result = build_json_from_input(input);

    TEST_ASSERT_EQUAL_INT(JSON_BUILD_ERROR_SYNTAX, result.status);
    TEST_ASSERT_NULL(result.json_string);

    free(result.json_string);
}

void test_build_json_update_stock_fails_invalid_quantity() {
    char input[] = "update_stock medicine bandages not_a_number";

    json_build_result result = build_json_from_input(input);

    TEST_ASSERT_EQUAL_INT(JSON_BUILD_ERROR_SYNTAX, result.status);
    TEST_ASSERT_NULL(result.json_string);

    free(result.json_string);
}

void test_parse_arguments_env_tcp_port() {
    setenv("CLIENT_PORT", "8000", 1);
    client_config config;
    const char *argv[] = {"./client", "localhost"};
    int argc = 2;

    TEST_ASSERT_TRUE(parse_arguments(argc, argv, &config));
    TEST_ASSERT_EQUAL_STRING("8000", config.port_tcp);
    TEST_ASSERT_EQUAL_STRING("8000", config.port_udp);

    unsetenv("CLIENT_PORT");
}

void test_parse_arguments_arguments_override_env_port() {
    setenv("CLIENT_PORT", "8000", 1);
    client_config config;
    const char *argv[] = {"./client", "localhost", "7000", "9000"};
    int argc = 4;

    TEST_ASSERT_TRUE(parse_arguments(argc, argv, &config));
    TEST_ASSERT_EQUAL_STRING("localhost", config.host);
    TEST_ASSERT_EQUAL_STRING("7000", config.port_tcp);
    TEST_ASSERT_EQUAL_STRING("9000", config.port_udp);

    unsetenv("CLIENT_PORT");
}

void test_parse_arguments_invalid_udp_args() {
    client_config config;
    const char *argv[] = {"./client", "localhost", "7000", "not_a_port"};
    int argc = 4;

    TEST_ASSERT_FALSE(parse_arguments(argc, argv, &config));
}

void test_parse_arguments_ports_from_one_arg() {
    client_config config;
    const char *argv[] = {"./client", "localhost", "7000"};
    int argc = 3;

    TEST_ASSERT_TRUE(parse_arguments(argc, argv, &config));
    TEST_ASSERT_EQUAL_STRING("localhost", config.host);
    TEST_ASSERT_EQUAL_STRING("7000", config.port_tcp);
    TEST_ASSERT_EQUAL_STRING("7000", config.port_udp);
}

void test_parse_arguments_invalid_env_tcp_port() {
    setenv("CLIENT_PORT", "not_a_port", 1);
    const char *argv[] = {"./client", "localhost"};
    int argc = 2;
    client_config config;

    bool result = parse_arguments(argc, argv, &config);

    TEST_ASSERT_FALSE(result);
    unsetenv("CLIENT_PORT");
}

void test_parse_arguments_invalid_env_udp_port() {
    setenv("CLIENT_PORT", "not_a_port", 1);
    const char *argv[] = {"./client", "localhost"};
    int argc = 2;
    client_config config;

    bool result = parse_arguments(argc, argv, &config);

    TEST_ASSERT_FALSE(result);
    unsetenv("CLIENT_PORT");
}