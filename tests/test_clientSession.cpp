#include "authenticator.hpp"
#include "clientSession.hpp"
#include "clock.hpp"
#include "config.hpp"
#include "inventory.hpp"
#include "test_helper.hpp"
#include "trafficReporter.hpp"
#include "unity.h"
#include <SQLiteCpp/Statement.h>
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void testSessionStartsUnauthenticated() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;

    clientSession session(-1, inventory, authenticator, logger, storage, "Some IP", sessionManager, dummyConfig,
                          trafficReporter);

    TEST_ASSERT_FALSE(session.isAuthenticated());
}

void testSessionAuthenticatesWithValidLogin() {

    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;

    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);

    storage.createUser("warehouse-A", "pass123");
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    clientSession::processResult result = session->processMessage(login_request);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Login successful! Welcome warehouse-A.", response["message"].get<std::string>().c_str());

    TEST_ASSERT_TRUE(result.second);
    TEST_ASSERT_TRUE(session->isAuthenticated());
    TEST_ASSERT_TRUE(sessionManager.isClientRegistered("warehouse-A"));
}

void testSessionAuthenticatesWithInvalidLogin() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);
    storage.createUser("warehouse-A", "pass123");

    // wrong password
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"wrong_password\"}}";

    clientSession::processResult result = session->processMessage(login_request);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Login failed. Invalid hostname or password.",
                             response["message"].get<std::string>().c_str());

    TEST_ASSERT_TRUE(result.second);
    TEST_ASSERT_FALSE(session->isAuthenticated());
}

void testSessionRejectsOtherCommandsWhenUnauthenticated() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);
    std::string status_request = "{\"command\":\"status\"}";

    clientSession::processResult result = session->processMessage(status_request);

    TEST_ASSERT_TRUE(result.second);
    const char *expected_response =
        "{\"message\":\"Authentication required. Please log in first.\",\"status\":\"error\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());

    TEST_ASSERT_FALSE(session->isAuthenticated());
}

/**
 * @brief Tests that processMessage correctly handles a malformed JSON string.
 */
void testProcessMessageHandlesMalformedJson() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);

    std::string malformed_request = "totally not a json format}";

    clientSession::processResult result = session->processMessage(malformed_request);

    // session should continue and give back error message
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"status\":\"error\",\"message\":\"Invalid JSON format.\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());
}

void testProcessMessageHandlesInvalidLoginRequest() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);

    std::string malformed_request = "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\"}}";

    clientSession::processResult result = session->processMessage(malformed_request);

    // session should continue and give back error message
    TEST_ASSERT_TRUE(result.second);
    const char *expected_response = "{\"message\":\"Malformed login request. Please provide a valid JSON with "
                                    "'payload', 'hostname' and 'password'.\",\"status\":\"error\"}";
    TEST_ASSERT_EQUAL_STRING(expected_response, result.first.c_str());
}

void testProcessMessageAuthenticatedCommand() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;

    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);
    storage.createUser("warehouse-A", "pass123");
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    clientSession::processResult result = session->processMessage(login_request);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Login successful! Welcome warehouse-A.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
    TEST_ASSERT_TRUE(session->isAuthenticated());

    std::string status_request = "{\"command\":\"status\"}";

    clientSession::processResult result2 = session->processMessage(status_request);

    json response2 = json::parse(result2.first);
    TEST_ASSERT_EQUAL_STRING("success", response2["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("STATUS: OK", response2["message"].get<std::string>().c_str());
}

void testProcessMessageCatchExceptionFromAuthenticatedCommand() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);

    storage.createUser("warehouse-A", "pass123");
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    clientSession::processResult result = session->processMessage(login_request);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("success", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Login successful! Welcome warehouse-A.", response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
    TEST_ASSERT_TRUE(session->isAuthenticated());
    TEST_ASSERT_TRUE(storage.userExists("warehouse-A"));

    // Sabotage the table
    storage.getDb().exec("DROP TABLE inventory;");

    std::string update_request = "{\"command\": \"update_stock\", \"payload\": {\"category\": \"food\", \"item\": "
                                 "\"water\", \"quantity\": 500}}";

    clientSession::processResult result2 = session->processMessage(update_request);

    json response2 = json::parse(result2.first);
    TEST_ASSERT_EQUAL_STRING("error", response2["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("An internal server error occurred. Please reconnect",
                             response2["message"].get<std::string>().c_str());
}

void testCreateLoggableRequestMasksPassword() {
    json original_request = json::parse(R"({"payload":{"password":"pass123"}})");

    json loggable_request = clientSession::createLoggableRequest(original_request);

    TEST_ASSERT_EQUAL_STRING("[REDACTED]", loggable_request["payload"]["password"].get<std::string>().c_str());
}

void testProcessMessageUserReachLimitFailedAttemps() {

    Storage storage(":memory:");
    storage.initializeSchema();
    Config dummyConfig = createDummyConfig();
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);

    storage.createUser("warehouse-A", "pass123");
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass124\"}}";

    clientSession::processResult result;
    result = session->processMessage(login_request);
    result = session->processMessage(login_request);
    result = session->processMessage(login_request);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Account is temporarily locked due to too many failed attempts.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
    TEST_ASSERT_FALSE(session->isAuthenticated());
}

void testProcessMessageUserLockedAlertTrigger() {

    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);

    storage.createUser("warehouse-A", "pass123");
    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    sessionManager.lockClient("warehouse-A");

    clientSession::processResult result;
    result = session->processMessage(login_request);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("Account is locked untill manually freed by an admin, due to alert trigger.",
                             response["message"].get<std::string>().c_str());
    TEST_ASSERT_TRUE(result.second);
    TEST_ASSERT_FALSE(session->isAuthenticated());
}

void testsetUdpAddress() {

    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);
    sockaddr_storage testAddr{};
    sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in *>(&testAddr);
    ipv4->sin_family = AF_INET;
    ipv4->sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &(ipv4->sin_addr));

    session->setUdpAddress(testAddr);

    auto storedAddr = session->getUdpAddress();
    TEST_ASSERT_NOT_NULL(storedAddr);

    const sockaddr_in *storedIpv4 = (const sockaddr_in *)storedAddr.get();
    TEST_ASSERT_EQUAL_INT(AF_INET, storedIpv4->sin_family);
    TEST_ASSERT_EQUAL_INT(htons(12345), storedIpv4->sin_port);
    TEST_ASSERT_EQUAL_UINT32(ipv4->sin_addr.s_addr, storedIpv4->sin_addr.s_addr);
}

void testClientSessionHandlesSQlExceptionOnLogin() {

    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    SessionManager sessionManager(storage, logger);
    TrafficReporter trafficReporter;
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);

    // Simulate a database error by dropping the users table
    storage.getDb().exec("DROP TABLE IF EXISTS users;");

    std::string login_request =
        "{\"command\":\"login\",\"payload\":{\"hostname\":\"warehouse-A\",\"password\":\"pass123\"}}";

    clientSession::processResult result = session->processMessage(login_request);
    json response = json::parse(result.first);

    TEST_ASSERT_EQUAL_STRING("error", response["status"].get<std::string>().c_str());
    TEST_ASSERT_EQUAL_STRING("An internal server error occurred. Please reconnect",
                             response["message"].get<std::string>().c_str());

    TEST_ASSERT_FALSE(result.second);
}