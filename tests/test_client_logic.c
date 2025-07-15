#include "client.h"
#include "unity.h"
#include <stdbool.h>
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
 * @brief Tests that setup_and_connect fails when given a non-numeric port.
 */
void test_setup_and_connect_fails_with_invalid_port_string() {

    int result_fd = setup_and_connect("localhost", "invalid port");

    TEST_ASSERT_EQUAL_INT(-1, result_fd);
}

/**
 * @brief Tests that build_message correctly formats a simple message.
 */
void test_build_message_normal_case() {
    char buffer[50];
    const char *message = "hello";

    build_message(buffer, 50, message);

    TEST_ASSERT_EQUAL_STRING("hello", buffer);
}

/**
 * @brief Tests that build_message safely truncates a message that is too long.
 *
 * This is a critical test for buffer overflow safety.
 */
void test_build_message_truncates_long_message() {
    char buffer[10];
    const char *long_message = "This message is definitely too long";

    build_message(buffer, 10, long_message);

    TEST_ASSERT_EQUAL_STRING("This mess", buffer);
    TEST_ASSERT_EQUAL_INT(9, strlen(buffer));
}