#include "authenticator.hpp"
#include "clientSession.hpp"
#include "inventory.hpp"
#include "unity.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void testSessionStartsUnauthenticated() {

    Inventory inventory;
    Authenticator authenticator;

    clientSession session(-1, inventory, authenticator);

    TEST_ASSERT_FALSE(session.isAuthenticated());
}

void testSessionAuthenticatesWithValidLogin() {

    Inventory inventory;
    Authenticator authenticator;
    clientSession session(-1, inventory, authenticator);

    // hardcoded login for now, must change later
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    clientSession::processResult result = session.processMessage(login_request);
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"success\",\"message\":\"Login successful.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());

    TEST_ASSERT_TRUE(session.isAuthenticated());
}

void testSessionAuthenticatesWithInvalidLogin() {

    Inventory inventory;
    Authenticator authenticator;
    clientSession session(-1, inventory, authenticator);

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

    Inventory inventory;
    Authenticator authenticator;
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

    Inventory inventory;
    Authenticator authenticator;
    clientSession session(-1, inventory, authenticator);

    std::string malformed_request = "totally not a json format}";

    clientSession::processResult result = session.processMessage(malformed_request);

    // session should continue and give back error message
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"error\",\"message\":\"Invalid JSON format.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());
}

void testProcessMessageHandlesInvalidLoginRequest() {

    Inventory inventory;
    Authenticator authenticator;
    clientSession session(-1, inventory, authenticator);

    std::string malformed_request = "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\"}}";

    clientSession::processResult result = session.processMessage(malformed_request);

    // session should continue and give back error message
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"error\",\"message\":\"Malformed login request.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());
}