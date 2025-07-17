#include "commandProcessor.hpp"
#include "unity.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void testProcessCommandStatusAsJSON() {
    std::string json_request = "{\"command\": \"status\"}";

    auto result = commandProcessor::processCommand(json_request, false);

    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("STATUS: OK", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandEndAsJSON() {
    std::string json_request = "{\"command\": \"end\"}";

    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Goodbye!", response["message"].get<std::string>().c_str());
    TEST_ASSERT_FALSE(result.second);
}

void testProcessCommandUnknownAsJSON() {
    std::string json_request = "{\"command\": \"unknown\"}";

    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("ACK: Unknown command received.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandUpdateStockAsJSON() {
    std::string json_request = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                               "\"water\", \"quantity\": 500}}";

    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Stock for 'food:water' successfully updated to 500.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandSrvrInMaintenance() {
    std::string json_request = "{\"command\": \"unknown\"}";

    auto result = commandProcessor::processCommand(json_request, true);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Server is in maintenance mode.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoCommand() {
    std::string json_request = "{\"Something\": \"unknown\"}";

    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Missing 'command' field in request.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoPayloadForUpload() {
    std::string json_request = "{\"command\": \"update_stock\", \"category\": \"food\", \"item\": "
                               "\"water\", \"quantity\": 500}";

    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoQuantityForUpload() {
    std::string json_request = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                               "\"water\"}}";
    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoItemForUpload() {
    std::string json_request =
        "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"quantity\": 500}}";
    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandNoCategoryForUpload() {
    std::string json_request = "{\"command\": \"update_stock\", \"payload\": {\"item\": \"water\", \"quantity\": 500}}";
    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandInvalidStringFormat() {
    std::string json_request = "not a JSON string";

    auto result = commandProcessor::processCommand(json_request, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid JSON format.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandUpdateStockInvalidQuantityType() {

    std::string request_str = "{\"command\":\"update_stock\",\"payload\":{\"category\":\"food\",\"item\":\"meat\","
                              "\"quantity\":\"onehundred\"}}";

    auto result = commandProcessor::processCommand(request_str, false);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid or missing field in update_stock payload.",
                             response["message"].get<std::string>().c_str());
}