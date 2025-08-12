#include "storage.hpp"
#include "unity.h"
#include <ctime>
#include <iostream>
#include <vector>

/**
 * @brief Tests that saveStockUpdate correctly inserts a new record.
 */
void testSaveStockUpdate() {
    std::string clientId = "warehouse-1";
    std::string category = "food";
    std::string item = "water";
    int quantity = 500;

    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    storage.saveStockUpdate(clientId, category, item, quantity);

    try {
        SQLite::Statement query(storage.getDb(),
                                "SELECT quantity FROM inventory WHERE client_id = ? AND category = ? AND item = ?");
        query.bind(1, clientId);
        query.bind(2, category);
        query.bind(3, item);

        // executeStep() true if there is a row to process
        if (query.executeStep()) {
            int fetched_quantity = query.getColumn("quantity");
            TEST_ASSERT_EQUAL_INT(quantity, fetched_quantity);
        } else {
            TEST_FAIL_MESSAGE("No record found for the specified item after update.");
        }
    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

/**
 * @brief Tests that saveStockUpdate correctly updates an existing record.
 */
void testSaveStockUpdateModifiesExistingRow() {
    std::string clientId = "warehouse-1";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    storage.saveStockUpdate("warehouse-1", "food", "water", 500);

    storage.saveStockUpdate("warehouse-1", "food", "water", 999);

    try {
        SQLite::Statement query(storage.getDb(),
                                "SELECT quantity FROM inventory WHERE client_id = 'warehouse-1' AND item = 'water'");
        if (query.executeStep()) {
            TEST_ASSERT_EQUAL_INT(999, query.getColumn("quantity").getInt());
        } else {
            TEST_FAIL_MESSAGE("Record not found after modification.");
        }
    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

/**
 * @brief Tests that initializeSchema correctly creates the 'inventory' table.
 */
void testInitializeSchemaCreatesTables() {

    Storage storage(":memory:");
    storage.initializeSchema();

    try {
        // sqlite_master contains all the tables created, ask for inventory table
        SQLite::Statement query(storage.getDb(),
                                "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='inventory'");

        if (query.executeStep()) {
            int table_count = query.getColumn(0); // getColumn(0) gets the first column of the result
            TEST_ASSERT_EQUAL_INT(1, table_count);
        } else {
            TEST_FAIL_MESSAGE("Query to check table existence failed to execute.");
        }
    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

/**
 * @brief Tests that saveStockUpdate throws an exception when a NOT NULL constraint is violated.
 */
void testSaveStockUpdateThrowsOnConstraintViolation() {

    Storage storage(":memory:");
    storage.initializeSchema();

    try {

        storage.saveStockUpdate("", "food", "meat", 100);

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");

    } catch (const SQLite::Exception &e) {

        TEST_PASS();

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

/**
 * @brief Tests that initializeSchema throws an exception if the DB connection is invalid.
 */
void testInitializeSchemaThrowsOnInvalidConnection() {
    Storage storageRO(":memory:", SQLite::OPEN_READONLY);

    try {
        storageRO.initializeSchema();

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but none was thrown.");

    } catch (const SQLite::Exception &e) {
        TEST_ASSERT_NOT_NULL(strstr(e.what(), "readonly"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");
    } catch (...) {
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testGetStockFromDataBase() {
    Storage storage(":memory:");

    try {
        storage.initializeSchema();
        storage.createUser("warehouse-1", "pass123");
        storage.saveStockUpdate("warehouse-1", "food", "water", 500);

        std::optional<int> result = storage.getStock("warehouse-1", "food", "water");
        TEST_ASSERT_TRUE(result.has_value());
        TEST_ASSERT_EQUAL_INT(500, *result);

    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}
void testGetStockEmptyForNoUser() {
    Storage storage(":memory:");

    try {
        storage.initializeSchema();
        storage.createUser("warehouse-1", "pass123");
        storage.saveStockUpdate("warehouse-1", "food", "water", 500);

        std::optional<int> result = storage.getStock("some_user", "food", "water");
        TEST_ASSERT_FALSE(result.has_value());

    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

void testGetStockReturnsEmptyForNonExistentItem() {

    Storage storage(":memory:");
    try {
        storage.initializeSchema();
        storage.createUser("warehouse-1", "pass123");
        std::optional<int> result = storage.getStock("warehouse-1", "food", "non_existent_item");

        TEST_ASSERT_FALSE(result.has_value());

    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

void testGetStockThrowsIfSchemaNotInitialized() {

    Storage storage(":memory:");
    try {

        storage.getStock("warehouse-1", "food", "meat");

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");

    } catch (const SQLite::Exception &e) {

        TEST_ASSERT_NOT_NULL(strstr(e.what(), "no such table: inventory"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testGetFullInventorySuccess() {
    Storage storage(":memory:");

    try {
        storage.initializeSchema();
        storage.createUser("warehouse-1", "pass123");
        storage.saveStockUpdate("warehouse-1", "food", "water", 500);
        storage.saveStockUpdate("warehouse-1", "medicine", "bandages", 1000);
        storage.saveStockUpdate("warehouse-1", "food", "meat", 100);
        auto result = storage.getFullInventory("warehouse-1");

        TEST_ASSERT_TRUE(result.has_value());
        // get json out from optional to work with
        Storage::ClientInventoryMap inventoryMap = *result;
        TEST_ASSERT_EQUAL_INT(500, inventoryMap["food"]["water"]);
        TEST_ASSERT_EQUAL_INT(1000, inventoryMap["medicine"]["bandages"]);
        TEST_ASSERT_EQUAL_INT(100, inventoryMap["food"]["meat"]);

    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

void testGetFullInventoryNoClientFound() {
    Storage storage(":memory:");

    try {
        storage.initializeSchema();
        storage.createUser("warehouse-1", "pass123");

        auto result = storage.getFullInventory("warehouse-2");

        TEST_ASSERT_FALSE(result.has_value());

    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

void testGetFullInventoryIsEmpty() {
    Storage storage(":memory:");

    try {
        storage.initializeSchema();
        storage.createUser("warehouse-1", "pass123");

        auto result = storage.getFullInventory("warehouse-1");

        TEST_ASSERT_FALSE(result.has_value());

    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

void testGetFullInventoryThrowsIfTableNotInitialized() {

    Storage storage(":memory:");
    try {

        storage.getFullInventory("some_client");

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");

    } catch (const SQLite::Exception &e) {

        TEST_ASSERT_NOT_NULL(strstr(e.what(), "no such table"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}
void testCreateUserSuccess() {
    Storage storage(":memory:");
    storage.initializeSchema();

    storage.createUser("Warehouse-A", "pass123");
    TEST_ASSERT_TRUE(storage.userExists("Warehouse-A"));
}

void testCreateUserThrows() {
    Storage storage(":memory:");
    storage.initializeSchema();

    try {

        storage.createUser("", "pass123"); // empty hostname should throw except
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");

    } catch (const SQLite::Exception &e) {

        TEST_ASSERT_NOT_NULL(strstr(e.what(), "CHECK constraint failed: length(hostname) > 0"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }

    TEST_ASSERT_TRUE(storage.userExists("Warehouse-A"));
}

void testCreateUserFailsOnDuplicate() {
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("warehouse-A", "pass");

    try {
        storage.createUser("warehouse-A", "pass2");
        TEST_FAIL_MESSAGE("Expected SQLite::Exception for duplicate hostname");

    } catch (const SQLite::Exception &e) {

        TEST_ASSERT_NOT_NULL(strstr(e.what(), "UNIQUE constraint failed"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");

    } catch (...) {
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testUserNotInTable() {
    Storage storage(":memory:");
    storage.initializeSchema();

    storage.createUser("Warehouse-A", "pass123");
    TEST_ASSERT_FALSE(storage.userExists("Warehouse-B"));
}

void testUserExistsThrowsWithNoTable() {
    Storage storage(":memory:");
    try {
        storage.userExists("anyhost");
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");
    } catch (const SQLite::Exception &e) {
        TEST_ASSERT_NOT_NULL(strstr(e.what(), "no such table"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");
    } catch (...) {
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testGetHostnamePasswordhash() {
    Storage storage(":memory:");
    storage.initializeSchema();

    storage.createUser("Warehouse-A", "pass123");
    std::optional<userAuthData> userData = storage.getUserLoginData("Warehouse-A");
    std::cout << "el user.data passwordhash: " << userData->passwordHash << '\n' << std::flush;
    TEST_ASSERT_TRUE(userData.has_value());
    TEST_ASSERT_EQUAL_INT(0, userData->failedAttempts);
}

void testNoHostnameforGetHostnamePasswordhash() {
    Storage storage(":memory:");
    storage.initializeSchema();

    storage.createUser("Warehouse-A", "pass123");
    std::optional<userAuthData> userData = storage.getUserLoginData("some hostname");

    TEST_ASSERT_FALSE(userData.has_value());
}

void testUpdateLoginAttempts() {
    std::string hostname = "Warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(hostname, "pass123");
    const std::time_t Time1 = 11111;
    storage.updateLoginAttempts(hostname, false, Time1);

    std::optional<userAuthData> result1 = storage.getUserLoginData(hostname);
    TEST_ASSERT_TRUE(result1.has_value());
    TEST_ASSERT_EQUAL_INT(1, result1->failedAttempts);
    TEST_ASSERT_EQUAL_INT(static_cast<long>(Time1), result1->lastFailedTimestamp);

    const std::time_t Time2 = 22222;
    storage.updateLoginAttempts(hostname, false, Time2);

    std::optional<userAuthData> result2 = storage.getUserLoginData(hostname);
    TEST_ASSERT_TRUE(result2.has_value());
    TEST_ASSERT_EQUAL_INT(2, result2->failedAttempts);
    TEST_ASSERT_EQUAL_INT(static_cast<long>(Time2), result2->lastFailedTimestamp);

    // success
    storage.updateLoginAttempts(hostname, true, 0);
    std::optional<userAuthData> result3 = storage.getUserLoginData(hostname);
    TEST_ASSERT_TRUE(result3.has_value());
    TEST_ASSERT_EQUAL_INT(0, result3->failedAttempts);

    TEST_ASSERT_EQUAL_INT(static_cast<long>(Time2), result3->lastFailedTimestamp);
}

void testUpdateLoginAttemptsUpToFour() {
    std::string hostname = "Warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(hostname, "pass123");
    const std::time_t Time1 = 11111;

    try {

        storage.updateLoginAttempts(hostname, false, Time1);
        storage.updateLoginAttempts(hostname, false, Time1 + 1);
        storage.updateLoginAttempts(hostname, false, Time1 + 2);
        storage.updateLoginAttempts(hostname, false, Time1 + 3);

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");

    } catch (const SQLite::Exception &e) {

        TEST_ASSERT_NOT_NULL(strstr(e.what(), "CHECK constraint failed: failed_attempts <= 3"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testUpdateLoginAttemptsResets() {
    std::string hostname = "Warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();

    storage.createUser(hostname, "pass123");
    storage.updateLoginAttempts(hostname, false, 1);
    storage.updateLoginAttempts(hostname, false, 2);
    storage.updateLoginAttempts(hostname, false, 3);
    storage.updateLoginAttempts(hostname, true, 0);

    std::optional<userAuthData> userData = storage.getUserLoginData(hostname);
    TEST_ASSERT_TRUE(userData.has_value());
    TEST_ASSERT_EQUAL_INT(0, userData->failedAttempts);
}

void testgetUserLoginThrows() {
    Storage storage(":memory:");
    try {
        storage.getUserLoginData("somehost");
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");
    } catch (const SQLite::Exception &e) {
        TEST_ASSERT_NOT_NULL(strstr(e.what(), "no such table"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");
    } catch (...) {
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testUserDoesntExists() {
    Storage storage(":memory:");
    storage.initializeSchema();
    TEST_ASSERT_FALSE(storage.userExists("somehostname"));
}

void testSaveLogEntrySuccess() {
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("Warehouse-1", "pass123");
    storage.saveLogEntry(5000, "INFO", "Authenticator", "User login successful", "Warehouse-1");
    SQLite::Statement query(storage.getDb(),
                            "SELECT timestamp, level, component, message FROM logs WHERE client_id = ?");
    query.bind(1, "Warehouse-1");

    TEST_ASSERT_TRUE(query.executeStep());
    TEST_ASSERT_EQUAL_INT(5000, query.getColumn(0));
    TEST_ASSERT_EQUAL_STRING("INFO", query.getColumn(1));
    TEST_ASSERT_EQUAL_STRING("Authenticator", query.getColumn(2));
    TEST_ASSERT_EQUAL_STRING("User login successful", query.getColumn(3));
}

void testSaveLogEntryThrowsWithInvalidUser() {
    Storage storage(":memory:");
    storage.initializeSchema();
    try {

        storage.saveLogEntry(5000, "INFO", "Authenticator", "User login successful", "some_user");

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");
    } catch (const SQLite::Exception &e) {
        TEST_ASSERT_NOT_NULL(strstr(e.what(), "FOREIGN KEY constraint failed"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");
    } catch (...) {
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testgetInventoryHistoryTransactionSuccessfull() {
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("Warehouse-1", "pass123");
    storage.saveLogEntry(5000, "INFO", "Inventory", "Stock updated for meat to 50", "Warehouse-1");
    storage.saveLogEntry(5001, "INFO", "Inventory", "Stock updated for water to 300", "Warehouse-1");

    std::vector<LogEntry> history = storage.getInventoryHistoryTransaction("Warehouse-1");

    TEST_ASSERT_EQUAL_INT(2, history.size());
    TEST_ASSERT_EQUAL_INT(5000, history[1].timestamp);
    TEST_ASSERT_EQUAL_STRING("INFO", history[1].level.c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory", history[1].component.c_str());
    TEST_ASSERT_EQUAL_STRING("Stock updated for meat to 50", history[1].message.c_str());

    TEST_ASSERT_EQUAL_INT(5001, history[0].timestamp);
    TEST_ASSERT_EQUAL_STRING("INFO", history[0].level.c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory", history[0].component.c_str());
    TEST_ASSERT_EQUAL_STRING("Stock updated for water to 300", history[0].message.c_str());
}

void testgetInventoryHistoryTransactionEmptyForNoLogs() {
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("Warehouse-1", "pass123");

    std::vector<LogEntry> history = storage.getInventoryHistoryTransaction("Warehouse-1");
    TEST_ASSERT_EQUAL_INT(0, history.size());
}

void testgetInventoryHistoryTransactionEmptyForNoUser() {
    Storage storage(":memory:");
    storage.initializeSchema();

    std::vector<LogEntry> history = storage.getInventoryHistoryTransaction("Warehouse-1");
    TEST_ASSERT_EQUAL_INT(0, history.size());
}

/**
 * @brief Tests that setClientLockStatus correctly updates the lock status of a user.
 */
void testSetClientLockStatus() {
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("warehouse-A", "pass123");

    TEST_ASSERT_FALSE(storage.isClientLocked("warehouse-A"));

    storage.setClientLockStatus("warehouse-A", true);

    TEST_ASSERT_TRUE(storage.isClientLocked("warehouse-A"));

    storage.setClientLockStatus("warehouse-A", false);

    TEST_ASSERT_FALSE(storage.isClientLocked("warehouse-A"));
}

/**
 * @brief Tests that isClientLocked correctly reads the lock status.
 */
void testIsClientLocked() {
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("user-unlocked", "pass123");
    storage.createUser("user-locked", "pass456");

    storage.setClientLockStatus("user-locked", true);

    TEST_ASSERT_FALSE(storage.isClientLocked("user-unlocked"));

    TEST_ASSERT_TRUE(storage.isClientLocked("user-locked"));

    TEST_ASSERT_FALSE(storage.isClientLocked("non-existent-user"));
}

void testGetInventoryHistoryThrowsNoTable() {
    Storage storage(":memory:");

    try {
        std::vector<LogEntry> history = storage.getInventoryHistoryTransaction("Warehouse-1");

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");

    } catch (const SQLite::Exception &e) {

        TEST_ASSERT_NOT_NULL(strstr(e.what(), "no such table"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testSetClientLockStatusThrowsNoTable() {
    Storage storage(":memory:");

    try {
        bool success = storage.setClientLockStatus("Warehouse-1", true);
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");

    } catch (const SQLite::Exception &e) {

        TEST_ASSERT_NOT_NULL(strstr(e.what(), "no such table"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}

void testIsClientLockedThrowsNoTable() {
    Storage storage(":memory:");

    try {
        bool success = storage.isClientLocked("Warehouse-1");
        TEST_FAIL_MESSAGE("Expected SQLite::Exception but no exception was thrown.");

    } catch (const SQLite::Exception &e) {

        TEST_ASSERT_NOT_NULL(strstr(e.what(), "no such table"));
        TEST_PASS_MESSAGE("Correctly caught expected exception.");

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected SQLite::Exception but a different exception was thrown.");
    }
}