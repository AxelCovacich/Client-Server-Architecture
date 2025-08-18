#include "alertManager.hpp"
#include "ipcHandler.hpp"
#include "logger.hpp"
#include "storage.hpp"
#include "unity.h"
#include <iostream>
#include <string>

void testIpcHandlerReadError() {

    Storage mockStorage(":memory:");
    SystemClock clock;
    Logger mockLogger(mockStorage, clock, std::cerr);
    SessionManager mockSessionManager(mockStorage, mockLogger);
    UdpHandler mockUdpHandler(-1, mockLogger, mockSessionManager);
    AlertManager mockAlertManager(mockLogger, mockSessionManager, mockUdpHandler);

    IpcHandler ipcHandler(mockLogger, mockAlertManager);
    int mockSocketFd = -1; // Simulate an invalid socket file descriptor
    bool result = ipcHandler.handleConnection(mockSocketFd);
    TEST_ASSERT_FALSE(result);
}