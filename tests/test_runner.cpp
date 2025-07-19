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
void test_build_json_for_simple_command();
void test_build_json_for_command_with_args();
void test_build_json_fails_on_missing_arguments();

} // extern "C"

void testProcessCommandStatusAsJSON();
void testProcessCommandEndAsJSON();
void testProcessCommandUnknownAsJSON();
void testServerConstructorFailsOnPrivilegedPort();
void testParserFailsWithInsufficientArgs();
void testParserFailsWithNonNumericPort();
void testParserSucceedsWithCorrectArgs();
void testProcessCommandUpdateStockAsJSON();
void testProcessCommandSrvrInMaintenance();
void testProcessCommandNoCommand();
void testProcessCommandNoPayloadForUpload();
void testParserInvalidPortNumber();
void testParserZeroNumberPort();
void testProcessCommandUpdateStockInvalidQuantityType();
void testProcessCommandNoQuantityForUpload();
void testProcessCommandNoCategoryForUpload();
void testProcessCommandNoItemForUpload();
void testUpdateStockAddsNewItem();
void testUpdateStockModifiesExistingItem();
void testGetInventorySummaryForUnknownClientReturnsEmptyJson();
void testGetInventorySummaryReturnsCorrectJson();
void testUpdateStockNegativeQuantity();
void testAuthenticatorFailsWithUnknownUser();
void testAuthenticatorFailsWithInvalidPassword();
void testAuthenticatorSucceedsWithValidCredentials();
void testSessionStartsUnauthenticated();
void testSessionAuthenticatesWithValidLogin();
void testSessionAuthenticatesWithInvalidLogin();
void testSessionRejectsOtherCommandsWhenUnauthenticated();
void testProcessMessageHandlesMalformedJson();
void testProcessCommandgetInventory();
void testProcessCommandgGetEmptyInventory();
void testProcessMessageHandlesInvalidLoginRequest();

/**
 * @brief Runs all the tests.
 *
 * Call all the tests for each c and cpp files in one runner(this file)
 *
 * @return 0 on success, non-zero on failure.
 */
int main() {

    UNITY_BEGIN();
    RUN_TEST(testProcessCommandStatusAsJSON);
    RUN_TEST(testProcessCommandEndAsJSON);
    RUN_TEST(testProcessCommandUnknownAsJSON);
    RUN_TEST(testServerConstructorFailsOnPrivilegedPort);
    RUN_TEST(test_process_input_returns_send_for_normal_message);
    RUN_TEST(test_process_input_returns_quit_for_end_command);
    RUN_TEST(test_process_input_returns_continue_for_empty_input);
    RUN_TEST(test_setup_and_connect_fails_with_invalid_port_string);
    RUN_TEST(testParserFailsWithInsufficientArgs);
    RUN_TEST(testParserFailsWithNonNumericPort);
    RUN_TEST(testParserSucceedsWithCorrectArgs);
    RUN_TEST(test_build_json_for_simple_command);
    RUN_TEST(test_build_json_for_command_with_args);
    RUN_TEST(test_build_json_fails_on_missing_arguments);
    RUN_TEST(testProcessCommandUpdateStockAsJSON);
    RUN_TEST(testProcessCommandSrvrInMaintenance);
    RUN_TEST(testProcessCommandNoCommand);
    RUN_TEST(testProcessCommandNoPayloadForUpload);
    RUN_TEST(testParserInvalidPortNumber);
    RUN_TEST(testParserZeroNumberPort);
    RUN_TEST(testProcessCommandUpdateStockInvalidQuantityType);
    RUN_TEST(testProcessCommandNoQuantityForUpload);
    RUN_TEST(testProcessCommandNoCategoryForUpload);
    RUN_TEST(testProcessCommandNoItemForUpload);
    RUN_TEST(testUpdateStockAddsNewItem);
    RUN_TEST(testUpdateStockModifiesExistingItem);
    RUN_TEST(testGetInventorySummaryReturnsCorrectJson);
    RUN_TEST(testGetInventorySummaryForUnknownClientReturnsEmptyJson);
    RUN_TEST(testUpdateStockNegativeQuantity);
    RUN_TEST(testAuthenticatorFailsWithUnknownUser);
    RUN_TEST(testAuthenticatorFailsWithInvalidPassword);
    RUN_TEST(testAuthenticatorSucceedsWithValidCredentials);
    RUN_TEST(testSessionStartsUnauthenticated);
    RUN_TEST(testSessionAuthenticatesWithValidLogin);
    RUN_TEST(testSessionAuthenticatesWithInvalidLogin);
    RUN_TEST(testSessionRejectsOtherCommandsWhenUnauthenticated);
    RUN_TEST(testProcessMessageHandlesMalformedJson);
    RUN_TEST(testProcessCommandgetInventory);
    RUN_TEST(testProcessCommandgGetEmptyInventory);
    RUN_TEST(testProcessMessageHandlesInvalidLoginRequest);

    // add more tests here;
    return UNITY_END();
}