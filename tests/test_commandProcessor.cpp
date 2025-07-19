#include "commandProcessor.hpp"
#include "inventory.hpp"
#include "unity.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void testProcessCommandStatusAsJSON() {
    std::string json_request = "{\"command\": \"status\"}";
    json request_object = json::parse(json_request);
    Inventory inventory;
    std::string clientId = "warehouse-A";

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("STATUS: OK", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandEndAsJSON() {
    std::string json_request = "{\"command\": \"end\"}";
    json request_object = json::parse(json_request);
    Inventory inventory;
    std::string clientId = "warehouse-A";

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Goodbye!", response["message"].get<std::string>().c_str());
    TEST_ASSERT_FALSE(result.second);
}

void testProcessCommandUnknownAsJSON() {
    std::string json_request = "{\"command\": \"unknown\"}";
    json request_object = json::parse(json_request);
    Inventory inventory;
    std::string clientId = "warehouse-A";

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("ACK: Unknown command received.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandUpdateStockAsJSON() {
    std::string json_request = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                               "\"water\", \"quantity\": 500}}";

    json request_object = json::parse(json_request);
    Inventory inventory;
    std::string clientId = "warehouse-A";
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Stock for 'food:water' successfully updated to 500.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandSrvrInMaintenance() {
    std::string json_request = "{\"command\": \"unknown\"}";
    std::string clientId = "warehouse-A";
    json request_object = json::parse(json_request);

    Inventory inventory;
    auto result = commandProcessor::processCommand(request_object, clientId, true, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Server is in maintenance mode.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoCommand() {
    std::string json_request = "{\"Something\": \"unknown\"}";
    json request_object = json::parse(json_request);

    std::string clientId = "warehouse-A";
    Inventory inventory;
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Missing 'command' field in request.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoPayloadForUpload() {
    std::string json_request = "{\"command\": \"update_stock\", \"category\": \"food\", \"item\": "
                               "\"water\", \"quantity\": 500}";

    json request_object = json::parse(json_request);

    std::string clientId = "warehouse-A";
    Inventory inventory;
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoQuantityForUpload() {
    std::string json_request = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                               "\"water\"}}";

    json request_object = json::parse(json_request);
    std::string clientId = "warehouse-A";
    Inventory inventory;
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoItemForUpload() {
    std::string json_request =
        "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"quantity\": 500}}";

    json request_object = json::parse(json_request);
    std::string clientId = "warehouse-A";
    Inventory inventory;
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoCategoryForUpload() {
    std::string json_request = "{\"command\": \"update_stock\", \"payload\": {\"item\": \"water\", \"quantity\": 500}}";

    json request_object = json::parse(json_request);
    std::string clientId = "warehouse-A";
    Inventory inventory;
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandUpdateStockInvalidQuantityType() {

    std::string request_str = "{\"command\":\"update_stock\",\"payload\":{\"category\":\"food\",\"item\":\"meat\","
                              "\"quantity\":\"onehundred\"}}";

    json request_object = json::parse(request_str);
    std::string clientId = "warehouse-A";
    Inventory inventory;
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
}

void testProcessCommandgetInventory() {

    std::string clientId = "warehouse-A";
    Inventory inventory;
    inventory.updateStock(clientId, "food", "meat", 100);
    inventory.updateStock(clientId, "medicine", "bandages", 250);
    std::string request_str = "{\"command\":\"get_inventory\"}";

    json request_object = json::parse(request_str);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory data for client warehouse-A retrieved.",
                             response["message"].get<std::string>().c_str());

    // check if data exists
    TEST_ASSERT_TRUE(response.contains("data"));

    json data = response["data"];
    TEST_ASSERT_EQUAL_INT(100, data["food"]["meat"].get<int>());
    TEST_ASSERT_EQUAL_INT(250, data["medicine"]["bandages"].get<int>());
}

void testProcessCommandgGetEmptyInventory() {

    std::string clientId = "warehouse-A";
    Inventory inventory;
    std::string request_str = "{\"command\":\"get_inventory\"}";

    json request_object = json::parse(request_str);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory data for client warehouse-A retrieved.",
                             response["message"].get<std::string>().c_str());

    // check if data exists
    TEST_ASSERT_TRUE(response.contains("data"));

    TEST_ASSERT_TRUE(response["data"].empty());
}