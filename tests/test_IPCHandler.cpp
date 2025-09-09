#include "alertManager.hpp"
#include "eventQueue.hpp"
#include "ipcHandler.hpp"
#include "logger.hpp"
#include "storage.hpp"
#include "test_helper.hpp"
#include "trafficReporter.hpp"
#include "unity.h"
#include <iostream>
#include <string>

void testIpcHandlerReadError() {

    Config dummyConfig = createDummyConfig();
    Storage mockStorage(":memory:");
    SystemClock clock;
    Logger mockLogger(mockStorage, clock, std::cerr, dummyConfig);
    TrafficReporter trafficreporter;
    SessionManager mockSessionManager(mockStorage, mockLogger, trafficreporter);
    EventQueue eventQueue(10, mockLogger);
    UdpHandler mockUdpHandler(-1, mockLogger, mockSessionManager, trafficreporter, eventQueue);
    AlertManager mockAlertManager(mockLogger, mockSessionManager, mockUdpHandler);

    IpcHandler ipcHandler(mockLogger, mockAlertManager, trafficreporter);
    int mockSocketFd = -1; // Simulate an invalid socket file descriptor
    bool result = ipcHandler.handleConnection(mockSocketFd);
    TEST_ASSERT_FALSE(result);
}