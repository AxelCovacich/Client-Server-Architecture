#include "authenticator.hpp"
#include "clientSession.hpp"
#include "clock.hpp"
#include "inventory.hpp"
#include "unity.h"
#include <SQLiteCpp/Statement.h>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void testSessionStartsUnauthenticated() {

    Storage storage(":memory:");
    storage.initializeSchema();

    Inventory inventory(storage);
    SystemClock clock;
    Authenticator authenticator(storage, clock);

    clientSession session(-1, inventory, authenticator);

    TEST_ASSERT_FALSE(session.isAuthenticated());
}

void testSessionAuthenticatesWithValidLogin() {

    Storage storage(":memory:");
    storage.initializeSchema();

    Inventory inventory(storage);
    SystemClock clock;
    Authenticator authenticator(storage, clock);
    clientSession session(-1, inventory, authenticator);

    storage.createUser("warehouse-A", "pass123");
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    clientSession::processResult result = session.processMessage(login_request);
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"success\",\"message\":\"Login successful.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());

    TEST_ASSERT_TRUE(session.isAuthenticated());
}

void testSessionAuthenticatesWithInvalidLogin() {

    Storage storage(":memory:");
    storage.initializeSchema();

    Inventory inventory(storage);
    SystemClock clock;
    Authenticator authenticator(storage, clock);
    clientSession session(-1, inventory, authenticator);
    storage.createUser("warehouse-A", "pass123");

    // wrong password
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"wrong_password\"}}";

    clientSession::processResult result = session.processMessage(login_request);
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"error\",\"message\":\"Login failed. Invalid credentials.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());

    TEST_ASSERT_FALSE(session.isAuthenticated());
}

void testSessionRejectsOtherCommandsWhenUnauthenticated() {

    Storage storage(":memory:");
    storage.initializeSchema();

    Inventory inventory(storage);
    SystemClock clock;
    Authenticator authenticator(storage, clock);
    clientSession session(-1, inventory, authenticator);
    std::string status_request = "{\"command\":\"status\"}";

    clientSession::processResult result = session.processMessage(status_request);

    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"error\",\"message\":\"Authentication required.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());

    TEST_ASSERT_FALSE(session.isAuthenticated());
}

/**
 * @brief Tests that processMessage correctly handles a malformed JSON string.
 */
void testProcessMessageHandlesMalformedJson() {

    Storage storage(":memory:");
    storage.initializeSchema();

    Inventory inventory(storage);
    SystemClock clock;
    Authenticator authenticator(storage, clock);
    clientSession session(-1, inventory, authenticator);

    std::string malformed_request = "totally not a json format}";

    clientSession::processResult result = session.processMessage(malformed_request);

    // session should continue and give back error message
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"error\",\"message\":\"Invalid JSON format.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());
}

void testProcessMessageHandlesInvalidLoginRequest() {

    Storage storage(":memory:");
    storage.initializeSchema();

    Inventory inventory(storage);
    SystemClock clock;
    Authenticator authenticator(storage, clock);
    clientSession session(-1, inventory, authenticator);

    std::string malformed_request = "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\"}}";

    clientSession::processResult result = session.processMessage(malformed_request);

    // session should continue and give back error message
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"error\",\"message\":\"Malformed login request.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());
}

void testProcessMessageAuthenticatedCommand() {

    Storage storage(":memory:");
    storage.initializeSchema();

    Inventory inventory(storage);
    SystemClock clock;
    Authenticator authenticator(storage, clock);
    clientSession session(-1, inventory, authenticator);
    storage.createUser("warehouse-A", "pass123");
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    clientSession::processResult result = session.processMessage(login_request);
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"success\",\"message\":\"Login successful.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());

    TEST_ASSERT_TRUE(session.isAuthenticated());

    std::string status_request = "{\"command\":\"status\"}";

    clientSession::processResult result2 = session.processMessage(status_request);

    json response = json::parse(result2.first);
    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("STATUS: OK", response["message"].get<std::string>().c_str());
}

void testProcessMessageCatchExceptionFromAuthenticatedCommand() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Inventory inventory(storage);
    SystemClock clock;
    Authenticator authenticator(storage, clock);
    clientSession session(-1, inventory, authenticator);

    storage.createUser("warehouse-A", "pass123");
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    clientSession::processResult result = session.processMessage(login_request);
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"success\",\"message\":\"Login successful.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());

    TEST_ASSERT_TRUE(session.isAuthenticated());
    TEST_ASSERT_TRUE(storage.userExists("warehouse-A"));

    // Sabotage the table
    storage.getDb().exec("DROP TABLE inventory;");

    std::string update_request = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                                 "\"water\", \"quantity\": 500}}";

    clientSession::processResult result2 = session.processMessage(update_request);

    json response = json::parse(result2.first);
    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("An internal server error occurred.", response["message"].get<std::string>().c_str());
}