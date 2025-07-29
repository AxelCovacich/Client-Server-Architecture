#include "unity.h"
#include <sqlite3.h>

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
void testGetInventorySummaryForUnknownClientReturnsEmpty();
void testGetInventorySummaryReturnsCorrectMap();
void testUpdateStockNegativeQuantity();
void testSessionStartsUnauthenticated();
void testSessionAuthenticatesWithValidLogin();
void testSessionAuthenticatesWithInvalidLogin();
void testSessionRejectsOtherCommandsWhenUnauthenticated();
void testProcessMessageHandlesMalformedJson();
void testProcessCommandgetInventory();
void testProcessCommandgGetEmptyInventory();
void testProcessMessageHandlesInvalidLoginRequest();
void testSaveStockUpdate();
void testSaveStockUpdateModifiesExistingRow();
void testInitializeSchemaCreatesTables();
void testParserFailsWithExcessiveArgs();
void testSaveStockUpdateThrowsOnConstraintViolation();
void testInitializeSchemaThrowsOnInvalidConnection();
void testGetStockFromDataBase();
void testGetStockReturnsEmptyForNonExistentItem();
void testProcessCommandGetStockSuccessfully();
void testProcessCommandGetStockNoItemFound();
void testProcessCommandInvalidPayloadForGetStock();
void testGetStockThrowsIfSchemaNotInitialized();
void testGetStockRetrievesFromDatabaseOnCacheMiss();
void testGetFullInventorySuccess();
void testGetFullInventoryNoClientFound();
void testGetFullInventoryThrowsIfTableNotInitialized();
void testCreateUserSuccess();
void testUserNotInTable();
void testGetHostnamePasswordhash();
void testNoHostnameforGetHostnamePasswordhash();
void testUpdateLoginAttempts();
void testUpdateLoginAttemptsUpToFour();
void testUpdateLoginAttemptsResets();
void testAuthenticatorSucceedsWithValidCredentials();
void testAuthenticatorFailsWithInvalidPassword();
void testAuthenticatorFailsWithUnknownUser();
void testAuthenticatorFailsWithFailedAttempts();
void testAuthenticatorResetsFailedAttempts();
void testProcessMessageAuthenticatedCommand();
void testProcessMessageCatchExceptionFromAuthenticatedCommand();
void testAuthenticatorPassAfterBlockedTime();
void testCreateUserThrows();
void testUserExistsThrowsWithNoTable();
void testgetUserLoginThrows();
void testCreateUserFailsOnDuplicate();
void testgetInventorySummaryFromDataBase();
void testUserDoesntExists();
void testSaveLogEntrySuccess();
void testSaveLogEntryThrowsWithInvalidUser();
void testSystemLog();
void testComponentLogWithClientID();
void testGetStockEmptyForNoUser();
void testLogFailsGracefullyWithInvalidClientId();
void testLogLevel();
void testCreateLoggableRequestMasksPassword();
void testProcessMessageUserReachLimitFailedAttemps();
void testProcessCommandUpdateStockNegativeQuantity();
void testgetInventoryHistoryTransactionSuccessfull();
void testgetInventoryHistoryTransactionEmptyForNoLogs();
void testgetInventoryHistoryTransactionEmptyForNoUser();
void testGetFullInventoryIsEmpty();
void testCommnadGetHistoryNoLogsEmptyData();
void testCommnadGetHistory();
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
    RUN_TEST(testGetInventorySummaryReturnsCorrectMap);
    RUN_TEST(testGetInventorySummaryForUnknownClientReturnsEmpty);
    RUN_TEST(testUpdateStockNegativeQuantity);
    RUN_TEST(testAuthenticatorSucceedsWithValidCredentials);
    RUN_TEST(testSessionStartsUnauthenticated);
    RUN_TEST(testSessionAuthenticatesWithValidLogin);
    RUN_TEST(testSessionAuthenticatesWithInvalidLogin);
    RUN_TEST(testSessionRejectsOtherCommandsWhenUnauthenticated);
    RUN_TEST(testProcessMessageHandlesMalformedJson);
    RUN_TEST(testProcessCommandgetInventory);
    RUN_TEST(testProcessCommandgGetEmptyInventory);
    RUN_TEST(testProcessMessageHandlesInvalidLoginRequest);
    RUN_TEST(testSaveStockUpdate);
    RUN_TEST(testSaveStockUpdateModifiesExistingRow);
    RUN_TEST(testInitializeSchemaCreatesTables);
    RUN_TEST(testParserFailsWithExcessiveArgs);
    RUN_TEST(testSaveStockUpdateThrowsOnConstraintViolation);
    RUN_TEST(testInitializeSchemaThrowsOnInvalidConnection);
    RUN_TEST(testGetStockFromDataBase);
    RUN_TEST(testGetStockEmptyForNoUser);
    RUN_TEST(testGetStockReturnsEmptyForNonExistentItem);
    RUN_TEST(testProcessCommandGetStockSuccessfully);
    RUN_TEST(testProcessCommandGetStockNoItemFound);
    RUN_TEST(testProcessCommandInvalidPayloadForGetStock);
    RUN_TEST(testGetStockThrowsIfSchemaNotInitialized);
    RUN_TEST(testGetStockRetrievesFromDatabaseOnCacheMiss);
    RUN_TEST(testGetFullInventorySuccess);
    RUN_TEST(testGetFullInventoryNoClientFound);
    RUN_TEST(testGetFullInventoryThrowsIfTableNotInitialized);
    RUN_TEST(testCreateUserSuccess);
    RUN_TEST(testUserNotInTable);
    RUN_TEST(testGetHostnamePasswordhash);
    RUN_TEST(testNoHostnameforGetHostnamePasswordhash);
    RUN_TEST(testUpdateLoginAttempts);
    RUN_TEST(testUpdateLoginAttemptsUpToFour);
    RUN_TEST(testUpdateLoginAttemptsResets);
    RUN_TEST(testAuthenticatorFailsWithInvalidPassword);
    RUN_TEST(testAuthenticatorFailsWithUnknownUser);
    RUN_TEST(testAuthenticatorFailsWithFailedAttempts);
    RUN_TEST(testAuthenticatorResetsFailedAttempts);
    RUN_TEST(testProcessMessageAuthenticatedCommand);
    RUN_TEST(testProcessMessageCatchExceptionFromAuthenticatedCommand);
    RUN_TEST(testAuthenticatorPassAfterBlockedTime);
    RUN_TEST(testCreateUserThrows);
    RUN_TEST(testUserExistsThrowsWithNoTable);
    RUN_TEST(testgetUserLoginThrows);
    RUN_TEST(testCreateUserFailsOnDuplicate);
    RUN_TEST(testgetInventorySummaryFromDataBase);
    RUN_TEST(testUserDoesntExists);
    RUN_TEST(testSaveLogEntrySuccess);
    RUN_TEST(testSaveLogEntryThrowsWithInvalidUser);
    RUN_TEST(testSystemLog);
    RUN_TEST(testComponentLogWithClientID);
    RUN_TEST(testLogFailsGracefullyWithInvalidClientId);
    RUN_TEST(testLogLevel);
    RUN_TEST(testCreateLoggableRequestMasksPassword);
    RUN_TEST(testProcessMessageUserReachLimitFailedAttemps);
    RUN_TEST(testProcessCommandUpdateStockNegativeQuantity);
    RUN_TEST(testgetInventoryHistoryTransactionSuccessfull);
    RUN_TEST(testgetInventoryHistoryTransactionEmptyForNoLogs);
    RUN_TEST(testgetInventoryHistoryTransactionEmptyForNoUser);
    RUN_TEST(testGetFullInventoryIsEmpty);
    RUN_TEST(testCommnadGetHistoryNoLogsEmptyData);
    RUN_TEST(testCommnadGetHistory);

    sqlite3_shutdown();
    return UNITY_END();
}