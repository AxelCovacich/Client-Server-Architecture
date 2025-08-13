#include "clock.hpp"
#include "commandProcessor.hpp"
#include "config.hpp"
#include "inventory.hpp"
#include "logger.hpp"
#include "test_helper.hpp"
#include "unity.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

class MockClock : public IClock {
  public:
    std::time_t now() const override {
        return m_time;
    }
    void set_time(std::time_t new_time) {
        m_time = new_time;
    }

  private:
    std::time_t m_time = 0;
};

using json = nlohmann::json;

void testProcessCommandStatusAsJSON() {
    std::string json_request = "{\"command\": \"status\"}";
    json request_object = json::parse(json_request);
    Config dummyConfig = createDummyConfig();
    Storage storage(":memory:");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    std::string clientId = "warehouse-A";
    SessionManager sessionManager(storage, logger);

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage,
                                                   sessionManager, dummyConfig);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("STATUS: OK", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandEndAsJSON() {
    std::string json_request = "{\"command\": \"end\"}";
    json request_object = json::parse(json_request);
    Storage storage(":memory:");
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    std::string clientId = "warehouse-A";
    SessionManager session(storage, logger);

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Goodbye!", response["message"].get<std::string>().c_str());
    TEST_ASSERT_FALSE(result.second);
}

void testProcessCommandUnknownAsJSON() {
    std::string json_request = "{\"command\": \"unknown\"}";
    json request_object = json::parse(json_request);
    Storage storage(":memory:");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    std::string clientId = "warehouse-A";
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("ACK: Unknown command received.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandUpdateStockAsJSON() {
    std::string json_request = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                               "\"water\", \"quantity\": 500}}";

    json request_object = json::parse(json_request);
    Storage storage(":memory:");
    storage.initializeSchema();

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    std::string clientId = "warehouse-A";
    storage.createUser(clientId, "pass123");
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
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

    Storage storage(":memory:");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, true, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Server is in maintenance mode.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoCommand() {
    std::string json_request = "{\"Something\": \"unknown\"}";
    json request_object = json::parse(json_request);

    std::string clientId = "warehouse-A";
    Storage storage(":memory:");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
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

    Storage storage(":memory:");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
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
    Storage storage(":memory:");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
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
    Storage storage(":memory:");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
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
    Storage storage(":memory:");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandUpdateStockInvalidQuantityType() {

    std::string request_str = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                              "\"water\", \"quantity\": \"not_a_number\"}}";

    json request_object = json::parse(request_str);
    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandUpdateStockNegativeQuantity() {

    std::string request_str = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                              "\"water\", \"quantity\": -500}}";

    json request_object = json::parse(request_str);
    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Update rejected: Quantity cannot be negative.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandgetInventory() {

    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);

    inventory.updateStock(clientId, "food", "meat", 100);
    inventory.updateStock(clientId, "medicine", "bandages", 250);

    std::string request_str = "{\"command\":\"get_inventory\"}";

    json request_object = json::parse(request_str);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory data for client warehouse-A retrieved.",
                             response["message"].get<std::string>().c_str());

    // check if data exists
    TEST_ASSERT_TRUE(response.contains("data"));

    json data = response["data"];
    TEST_ASSERT_EQUAL_INT(100, data["food"]["meat"].get<int>());
    TEST_ASSERT_EQUAL_INT(250, data["medicine"]["bandages"].get<int>());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandgGetEmptyInventory() {

    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    std::string request_str = "{\"command\":\"get_inventory\"}";
    storage.createUser(clientId, "pass123");
    json request_object = json::parse(request_str);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory data for client warehouse-A is empty.",
                             response["message"].get<std::string>().c_str());

    // check if data IS EMPTY
    TEST_ASSERT_TRUE(response["data"].empty());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandGetStockSuccessfully() {
    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);

    inventory.updateStock(clientId, "food", "meat", 100);
    std::string request_str = "{\"command\":\"get_stock\",\"payload\":{\"category\":\"food\",\"item\":\"meat\"}}";
    json request_object = json::parse(request_str);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Stock data retrieved successfully.", response["message"].get<std::string>().c_str());

    TEST_ASSERT_TRUE(response.contains("data"));
    TEST_ASSERT_EQUAL_INT(100, response["data"]["quantity"].get<int>());
    TEST_ASSERT_EQUAL_STRING("food", response["data"]["category"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("meat", response["data"]["item"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandGetStockNoItemFound() {
    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    storage.createUser(clientId, "pass123");
    // inventory.updateStock(clientId, "food", "water", 100);
    std::string request_str = "{\"command\":\"get_stock\",\"payload\":{\"category\":\"food\",\"item\":\"meat\"}}";
    json request_object = json::parse(request_str);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Item not found for the specified client/category.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandInvalidPayloadForGetStock() {
    std::string json_request = "{\"command\": \"get_stock\", \"payload\": {\"item\": \"water\"}}";

    json request_object = json::parse(json_request);
    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();

    SystemClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in get_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testCommnadGetHistory() {
    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");

    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);

    clock.set_time(100);
    inventory.updateStock(clientId, "food", "meat", 100);
    clock.set_time(200);
    inventory.updateStock(clientId, "medicine", "bandages", 250);

    std::string request_str = "{\"command\":\"get_history\"}";

    json request_object = json::parse(request_str);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory history retrieved.", response["message"].get<std::string>().c_str());

    // check if data exists
    TEST_ASSERT_TRUE(response.contains("data"));
    TEST_ASSERT_TRUE(response["data"].is_array());
    TEST_ASSERT_FALSE(response["data"].empty());

    json data = response["data"];
    std::string expectedTimeStamp = "1970-01-01 00:03:20 UTC"; // 100 sec after epoch

    TEST_ASSERT_EQUAL_STRING(expectedTimeStamp.c_str(), data[0]["timestamp"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory", data[0]["component"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("INFO", data[0]["level"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Stock succesfully updated for medicine:bandages to 250 for client warehouse-A",
                             data[0]["message"].get<std::string>().c_str());

    std::string expectedTimeStamp2 = "1970-01-01 00:01:40 UTC"; // 200 sec after epoch
    TEST_ASSERT_EQUAL_STRING(expectedTimeStamp2.c_str(), data[1]["timestamp"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory", data[1]["component"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("INFO", data[1]["level"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Stock succesfully updated for food:meat to 100 for client warehouse-A",
                             data[1]["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testCommnadGetHistoryNoLogsEmptyData() {
    std::string clientId = "warehouse-A";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");

    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    Config dummyConfig = createDummyConfig();
    SessionManager session(storage, logger);

    std::string request_str = "{\"command\":\"get_history\"}";

    json request_object = json::parse(request_str);
    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage, session,
                                                   dummyConfig);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Inventory history retrieved.", response["message"].get<std::string>().c_str());

    TEST_ASSERT_TRUE(response.contains("data"));
    TEST_ASSERT_TRUE(response["data"].is_array());
    TEST_ASSERT_TRUE(response["data"].empty());
    TEST_ASSERT_TRUE(result.second);
}

void testUnlockClientWithCorrectSecretPhrase() {

    std::string clientId = "admin";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    storage.createUser("blocked_user", "pass123");

    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    SessionManager sessionManager(storage, logger);
    createTempYamlFile(R"(
server:
  port: 9999
database:
  path: ":memory:"
security:
  unlock_secret_phrase: "secret_phrase"
)");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    std::string request_str = R"({
        "command": "unlock_client",
        "payload": {
            "client_to_unlock": "blocked_user",
            "secret_phrase": "secret_phrase"
        }
    })";
    json request_object = json::parse(request_str);

    storage.setClientLockStatus("blocked_user", true); // block the user to unlockit with command and secret phrase

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage,
                                                   sessionManager, config);

    json response = json::parse(result.first);
    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Client `blocked_user` successfully unlocked.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_FALSE(storage.isClientLocked("blocked_user"));
    remove("./temp_config.yaml");
}

void testUnlockClientWrongClient() {

    std::string clientId = "admin";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    // storage.createUser("blocked_user","pass123");

    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    SessionManager sessionManager(storage, logger);
    createTempYamlFile(R"(
server:
  port: 9999
database:
  path: ":memory:"
security:
  unlock_secret_phrase: "secret_phrase"
)");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);
    std::string request_str = R"({
        "command": "unlock_client",
        "payload": {
            "client_to_unlock": "blocked_user",
            "secret_phrase": "secret_phrase"
        }
    })";
    json request_object = json::parse(request_str);

    // storage.setClientLockStatus("blocked_user",true);   //block the user to unlockit with command and secret phrase

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage,
                                                   sessionManager, config);

    json response = json::parse(result.first);
    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Couldn't unlock Client `blocked_user`. Client didn't found or misspelled.",
                             response["message"].get<std::string>().c_str());
}

void testUnlockClientWrongPhrase() {

    std::string clientId = "admin";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    // storage.createUser("blocked_user","pass123");

    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    SessionManager sessionManager(storage, logger);
    createTempYamlFile(R"(
server:
  port: 9999
database:
  path: ":memory:"
security:
  unlock_secret_phrase: "secret_phrase"
)");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    std::string request_str = R"({
        "command": "unlock_client",
        "payload": {
            "client_to_unlock": "blocked_user",
            "secret_phrase": "wrong_phrase"
        }
    })";
    json request_object = json::parse(request_str);

    // storage.setClientLockStatus("blocked_user",true);   //block the user to unlockit with command and secret phrase

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage,
                                                   sessionManager, config);

    json response = json::parse(result.first);
    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Wrong secret phrase. Try again.", response["message"].get<std::string>().c_str());
}

void testUnlockClientWrongPayload() {

    std::string clientId = "admin";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    // storage.createUser("blocked_user","pass123");

    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    SessionManager sessionManager(storage, logger);
    Config dummyConfig = createDummyConfig();

    std::string request_str = R"({
        "command": "unlock_client",
        "payload": {
            "client_to_unlock": "blocked_user"
        }
    })";
    json request_object = json::parse(request_str);

    // storage.setClientLockStatus("blocked_user",true);   //block the user to unlockit with command and secret phrase

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage,
                                                   sessionManager, dummyConfig);

    json response = json::parse(result.first);
    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in unlock_client payload.",
                             response["message"].get<std::string>().c_str());
}

void testUnlockClientNotAdminUser() {

    std::string clientId = "Not_Admin";
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser(clientId, "pass123");
    storage.createUser("blocked_user", "pass123");

    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Inventory inventory(storage, logger);
    SessionManager sessionManager(storage, logger);
    Config dummyConfig = createDummyConfig();

    std::string request_str = R"({
        "command": "unlock_client",
        "payload": {
            "client_to_unlock": "blocked_user",
            "secret_phrase": "secret_phrase"
        }
    })";
    json request_object = json::parse(request_str);

    // storage.setClientLockStatus("blocked_user",true);   //block the user to unlockit with command and secret phrase

    auto result = commandProcessor::processCommand(request_object, clientId, false, inventory, logger, storage,
                                                   sessionManager, dummyConfig);

    json response = json::parse(result.first);
    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Command only available for admin user.", response["message"].get<std::string>().c_str());
}