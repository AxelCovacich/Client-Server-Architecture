#include "inventory.hpp"
#include "storage.hpp"
#include "unity.h"
#include <optional>

/**
 * @brief Tests that updateStock correctly adds a new item to the inventory.
 */
void testUpdateStockAddsNewItem() {
    // database only in ram for testing
    Storage storage(":memory:");
    storage.initializeSchema();
    std::string clientId = "warehouse-A";
    Inventory inventory(storage);
    storage.createUser(clientId, "pass123");
    // Check inicial stock is empty
    std::optional<int> initial_stock = inventory.getStock("warehouse-A", "food", "meat");
    TEST_ASSERT_FALSE(initial_stock.has_value());

    // update stock
    inventory.updateStock("warehouse-A", "food", "meat", 150);

    //  Assert: check updated value
    std::optional<int> final_stock = inventory.getStock("warehouse-A", "food", "meat");
    TEST_ASSERT_EQUAL_INT(150, final_stock.value());

    //  missing category for client
    std::optional<int> CategoryStock = inventory.getStock("warehouse-A", "medicine", "meat");
    TEST_ASSERT_FALSE(CategoryStock.has_value());

    //  missing item for category
    std::optional<int> ItemStock = inventory.getStock("warehouse-A", "food", "water");
    TEST_ASSERT_FALSE(ItemStock.has_value());
}

/**
 * @brief Tests that updateStock correctly modifies an existing item's quantity.
 */
void testUpdateStockModifiesExistingItem() {
    Storage storage(":memory:");
    storage.initializeSchema();
    std::string clientId = "warehouse-A";
    Inventory inventory(storage);
    storage.createUser(clientId, "pass123");
    //  inicial stock
    inventory.updateStock("warehouse-A", "food", "meat", 150);

    // update old value with another
    inventory.updateStock("warehouse-A", "food", "meat", 99);

    // check for last value updated
    std::optional<int> final_stock = inventory.getStock("warehouse-A", "food", "meat");
    TEST_ASSERT_EQUAL_INT(99, final_stock.value());
}

void testGetInventorySummaryReturnsCorrectJson() {
    Storage storage(":memory:");
    storage.initializeSchema();
    std::string clientId = "warehouse-A";
    Inventory inventory(storage);
    storage.createUser(clientId, "pass123");
    // first we add items to inventory
    inventory.updateStock("warehouse-A", "food", "meat", 150);
    inventory.updateStock("warehouse-A", "medicine", "bandages", 300);

    std::string result_str = inventory.getInventorySummaryJson("warehouse-A");

    const char *expected_str = "{\"food\":{\"meat\":150},\"medicine\":{\"bandages\":300}}";
    TEST_ASSERT_EQUAL_STRING(expected_str, result_str.c_str());
}

void testGetInventorySummaryForUnknownClientReturnsEmptyJson() {
    Storage storage(":memory:");
    storage.initializeSchema();
    std::string clientId = "warehouse-A";
    Inventory inventory(storage);
    storage.createUser(clientId, "pass123");
    std::string result_str = inventory.getInventorySummaryJson("warehouse-B");
    TEST_ASSERT_EQUAL_STRING("{}", result_str.c_str());
}

void testUpdateStockNegativeQuantity() {
    Storage storage(":memory:");
    storage.initializeSchema();
    std::string clientId = "warehouse-A";
    Inventory inventory(storage);
    storage.createUser(clientId, "pass123");
    inventory.updateStock("warehouse-A", "food", "meat", 0);
    bool result = inventory.updateStock("warehouse-A", "food", "meat", -50);
    TEST_ASSERT_FALSE(result);

    // test the actual stock didn't change
    std::optional<int> final_stock = inventory.getStock("warehouse-A", "food", "meat");
    TEST_ASSERT_EQUAL_INT(0, final_stock.value());
}

/**
 * @brief Tests the cache-miss scenario for getStock.
 *
 * This test verifies that if an item is not in the in-memory cache but exists
 * in the database, getStock correctly retrieves it from the Storage layer.
 */
void testGetStockRetrievesFromDatabaseOnCacheMiss() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Inventory inventory(storage);
    std::string clientId = "warehouse-A";
    storage.createUser(clientId, "pass123");
    storage.saveStockUpdate("warehouse-A", "medicine", "bandages", 777);

    std::optional<int> result = inventory.getStock("warehouse-A", "medicine", "bandages");

    TEST_ASSERT_TRUE(result.has_value());
    TEST_ASSERT_EQUAL_INT(777, *result);
}

void testgetInventorySummaryFromDataBase() {
    Storage storage(":memory:");
    storage.initializeSchema();
    Inventory inventory(storage);
    std::string clientId = "warehouse-A";
    storage.createUser(clientId, "pass123");
    storage.saveStockUpdate(clientId, "medicine", "bandages", 300);
    storage.saveStockUpdate(clientId, "food", "meat", 150);

    std::string result_str = inventory.getInventorySummaryJson(clientId);
    const char *expected_str = "{\"food\":{\"meat\":150},\"medicine\":{\"bandages\":300}}";
    TEST_ASSERT_EQUAL_STRING(expected_str, result_str.c_str());
}