#include "authenticator.hpp"
#include "clientSession.hpp"
#include "clock.hpp"
#include "inventory.hpp"
#include "logger.hpp"
#include "sessionManager.hpp"
#include "storage.hpp"
#include "test_helper.hpp"
#include "trafficReporter.hpp"
#include "unity.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>

/**
 * @brief Tests the full lock and unlock lifecycle of a client.
 */
void testLockAndUnlockClient() {
    Config dummyConfig = createDummyConfig();
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("warehouse-A", "pass123");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    TrafficReporter trafficReporter;
    SessionManager sessionManager(storage, logger, trafficReporter);

    TEST_ASSERT_FALSE(sessionManager.isClientLocked("warehouse-A"));

    sessionManager.lockClient("warehouse-A");

    TEST_ASSERT_TRUE(sessionManager.isClientLocked("warehouse-A"));
    TEST_ASSERT_TRUE(storage.isClientLocked("warehouse-A"));

    storage.setClientLockStatus("warehouse-A", false);

    SessionManager newSessionManager(storage, logger, trafficReporter);
    TEST_ASSERT_FALSE(newSessionManager.isClientLocked("warehouse-A"));
}

void testLockNonExistentClient() {
    Config dummyConfig = createDummyConfig();
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("warehouse-A", "pass123");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    TrafficReporter trafficReporter;
    SessionManager sessionManager(storage, logger, trafficReporter);

    bool success = sessionManager.lockClient("Non_existent_user");
    TEST_ASSERT_FALSE(success);
}
/**
 * @brief Tests that isClientLocked correctly handles cache misses.
 */
void testIsClientLockedHandlesCacheMiss() {

    Config dummyConfig = createDummyConfig();
    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("warehouse-B", "pass123");
    storage.setClientLockStatus("warehouse-B", true);
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, dummyConfig);
    TrafficReporter trafficReporter;
    SessionManager sessionManager(storage, logger, trafficReporter);

    TEST_ASSERT_TRUE(sessionManager.isClientLocked("warehouse-B"));

    TEST_ASSERT_TRUE(sessionManager.isClientLocked("warehouse-B"));
}

void testRegisterAndUnregisterSession() {

    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    TrafficReporter trafficReporter;
    SessionManager sessionManager(storage, logger, trafficReporter);
    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);
    std::string clientId = "warehouse-A";

    TEST_ASSERT_FALSE(sessionManager.isClientRegistered(clientId));

    sessionManager.registerSession(clientId, session);

    TEST_ASSERT_TRUE(sessionManager.isClientRegistered(clientId));

    sessionManager.unregisterSession(clientId);

    TEST_ASSERT_FALSE(sessionManager.isClientRegistered(clientId));
}

void testgetActiveUdpAddresses() {
    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    TrafficReporter trafficReporter;
    SessionManager sessionManager(storage, logger, trafficReporter);

    // first session
    auto session1 = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "192.168.0.10",
                                                    sessionManager, dummyConfig, trafficReporter);

    struct sockaddr_in addr1 {};
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(12345);
    inet_pton(AF_INET, "192.168.0.10", &addr1.sin_addr);

    struct sockaddr_storage storageAddr1 {};
    memcpy(&storageAddr1, &addr1, sizeof(addr1));
    session1->setUdpAddress(storageAddr1);

    // second session
    auto session2 = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "10.0.0.5",
                                                    sessionManager, dummyConfig, trafficReporter);

    struct sockaddr_in addr2 {};
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(54321);
    inet_pton(AF_INET, "10.0.0.5", &addr2.sin_addr);

    struct sockaddr_storage storageAddr2 {};
    memcpy(&storageAddr2, &addr2, sizeof(addr2));
    session2->setUdpAddress(storageAddr2);

    sessionManager.registerSession("warehouse-1", session1);
    sessionManager.registerSession("warehouse-2", session2);

    auto result = sessionManager.getActiveUdpAddresses();

    TEST_ASSERT_EQUAL_INT(2, result.size());

    auto *res1 = (struct sockaddr_in *)&result[0];
    TEST_ASSERT_EQUAL_INT(AF_INET, res1->sin_family);
    TEST_ASSERT_EQUAL_UINT16(htons(12345), res1->sin_port);
    struct in_addr expected_ip1;
    inet_pton(AF_INET, "192.168.0.10", &expected_ip1);
    TEST_ASSERT_EQUAL_UINT32(expected_ip1.s_addr, res1->sin_addr.s_addr);

    auto *res2 = (struct sockaddr_in *)&result[1];
    TEST_ASSERT_EQUAL_INT(AF_INET, res2->sin_family);
    TEST_ASSERT_EQUAL_UINT16(htons(54321), res2->sin_port);
    struct in_addr expected_ip2;
    inet_pton(AF_INET, "10.0.0.5", &expected_ip2);
    TEST_ASSERT_EQUAL_UINT32(expected_ip2.s_addr, res2->sin_addr.s_addr);
}

void testSessionManagerGetClientSession() {

    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    Inventory inventory(storage, logger);
    Authenticator authenticator(storage, clock, logger);
    TrafficReporter trafficReporter;
    SessionManager sessionManager(storage, logger, trafficReporter);

    auto session = std::make_shared<clientSession>(-1, inventory, authenticator, logger, storage, "Some IP",
                                                   sessionManager, dummyConfig, trafficReporter);

    sessionManager.registerSession("warehouse-1", session);

    auto result = sessionManager.getSession("warehouse-1");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(session.get(), result.get());

    auto missing = sessionManager.getSession("someuser");
    TEST_ASSERT_NULL(missing);
}