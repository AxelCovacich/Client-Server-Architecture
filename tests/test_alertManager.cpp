// tests/test_alertManager.cpp

#include "alertManager.hpp"
#include "clock.hpp"
#include "eventQueue.hpp"
#include "logger.hpp"
#include "sessionManager.hpp"
#include "storage.hpp"
#include "test_helper.hpp"
#include "trafficReporter.hpp"
#include "udpHandler.hpp"
#include "unity.h"
#include <iostream>
#include <nlohmann/json.hpp>

/**
 * @brief Tests that processAlert successfully locks the correct client.
 */
void testProcessAlertLocksClientOnValidAlert() {

    Storage storage(":memory:");
    storage.initializeSchema();
    storage.createUser("warehouse-A", "pass123");
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    TrafficReporter trafficreporter;
    SessionManager sessionManager(storage, logger, trafficreporter);
    EventQueue eventQueue(10);
    UdpHandler udpHandler(-1, logger, sessionManager, trafficreporter, eventQueue);
    AlertManager alertManager(logger, sessionManager, udpHandler);

    std::string valid_alert = R"({
        "type": "infection",
        "message": "Spores detected.",
        "sensor_id": "sensor-01",
        "clientId": "warehouse-A"
    })";

    alertManager.processAlert(valid_alert);

    TEST_ASSERT_TRUE(sessionManager.isClientLocked("warehouse-A"));
}

/**
 * @brief Tests that processAlert handles a malformed JSON without crashing.
 */
void testProcessAlertHandlesMalformedJsonGracefully() {
    // --- Arrange ---
    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    TrafficReporter trafficreporter;
    SessionManager sessionManager(storage, logger, trafficreporter);
    EventQueue eventQueue(10);
    UdpHandler udpHandler(-1, logger, sessionManager, trafficreporter, eventQueue);
    AlertManager alertManager(logger, sessionManager, udpHandler);

    std::string malformed_alert = "not a json message";

    alertManager.processAlert(malformed_alert);

    TEST_PASS(); // if the test reach this line without crashing is a success.
}