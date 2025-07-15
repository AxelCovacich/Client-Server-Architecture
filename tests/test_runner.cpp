#include "unity.h"

extern "C" {

/**
 * @brief Unity hook called before each test.
 */
void setUp(void) {
}

/**
 * @brief Unity hook called after each test.
 */
void tearDown(void) {
}

/* Prototypes for C tests */

void test_process_input_returns_send_for_normal_message();
void test_process_input_returns_quit_for_end_command();
void test_process_input_returns_continue_for_empty_input();
void test_setup_and_connect_fails_with_invalid_port_string();
void test_build_message_normal_case();
void test_build_message_truncates_long_message();

} // extern "C"

void testProcessCommandStatus();
void testProcessCommandEnd();
void testProcessCommandUnknown();
void testServerConstructorFailsOnPrivilegedPort();
void testParserFailsWithInsufficientArgs();
void testParserFailsWithNonNumericPort();
void testParserSucceedsWithCorrectArgs();

/**
 * @brief Runs all the tests.
 *
 * Call all the tests for each c and cpp files in one runner(this file)
 *
 * @return 0 on success, non-zero on failure.
 */
int main() {

    UNITY_BEGIN();
    RUN_TEST(testProcessCommandStatus);
    RUN_TEST(testProcessCommandEnd);
    RUN_TEST(testProcessCommandUnknown);
    RUN_TEST(testServerConstructorFailsOnPrivilegedPort);
    RUN_TEST(test_process_input_returns_send_for_normal_message);
    RUN_TEST(test_process_input_returns_quit_for_end_command);
    RUN_TEST(test_process_input_returns_continue_for_empty_input);
    RUN_TEST(test_setup_and_connect_fails_with_invalid_port_string);
    RUN_TEST(test_build_message_normal_case);
    RUN_TEST(test_build_message_truncates_long_message);
    RUN_TEST(testParserFailsWithInsufficientArgs);
    RUN_TEST(testParserFailsWithNonNumericPort);
    RUN_TEST(testParserSucceedsWithCorrectArgs);
    // add more tests here;
    return UNITY_END();
}