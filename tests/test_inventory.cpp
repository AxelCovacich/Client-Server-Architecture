#include "inventory.hpp"
#include "unity.h"

/**
 * @brief Tests that updateStock correctly adds a new item to the inventory.
 */
void testUpdateStockAddsNewItem() {
    Inventory inventory;

    // Check inicial stock is 0
    int initial_stock = inventory.getStock("warehouse-A", "food", "meat");
    TEST_ASSERT_EQUAL_INT(0, initial_stock);

    // update stock
    inventory.updateStock("warehouse-A", "food", "meat", 150);

    //  Assert: check updated value
    int final_stock = inventory.getStock("warehouse-A", "food", "meat");
    TEST_ASSERT_EQUAL_INT(150, final_stock);

    //  missing category for client
    int CategoryStock = inventory.getStock("warehouse-A", "medicine", "meat");
    TEST_ASSERT_EQUAL_INT(0, CategoryStock);

    //  missing item for category
    int ItemStock = inventory.getStock("warehouse-A", "food", "water");
    TEST_ASSERT_EQUAL_INT(0, ItemStock);
}

/**
 * @brief Tests that updateStock correctly modifies an existing item's quantity.
 */
void testUpdateStockModifiesExistingItem() {
    Inventory inventory;

    //  inicial stock
    inventory.updateStock("warehouse-A", "food", "meat", 150);

    // update old value with another
    inventory.updateStock("warehouse-A", "food", "meat", 99);

    // check for last value updated
    int final_stock = inventory.getStock("warehouse-A", "food", "meat");
    TEST_ASSERT_EQUAL_INT(99, final_stock);
}

void testGetInventorySummaryReturnsCorrectJson() {
    Inventory inventory;
    // first we add items to inventory
    inventory.updateStock("warehouse-A", "food", "meat", 150);
    inventory.updateStock("warehouse-A", "medicine", "bandages", 300);

    std::string result_str = inventory.getInventorySummaryJson("warehouse-A");

    const char *expected_str = "{\"food\":{\"meat\":150},\"medicine\":{\"bandages\":300}}";
    TEST_ASSERT_EQUAL_STRING(expected_str, result_str.c_str());
}

void testGetInventorySummaryForUnknownClientReturnsEmptyJson() {
    Inventory inventory;
    std::string result_str = inventory.getInventorySummaryJson("warehouse-B");
    TEST_ASSERT_EQUAL_STRING("{}", result_str.c_str());
}

void testUpdateStockNegativeQuantity() {
    Inventory inventory;

    bool result = inventory.updateStock("warehouse-A", "food", "meat", -50);
    TEST_ASSERT_FALSE(result);

    // test the actual stock didn't change
    int final_stock = inventory.getStock("warehouse-A", "food", "meat");
    TEST_ASSERT_EQUAL_INT(0, final_stock);
}