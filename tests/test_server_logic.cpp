#include "server.hpp"
#include "unity.h"
#include <cstring>
#include <stdexcept>
#include <string>

#include <iostream>

using namespace std;

void testServerConstructorFailsOnPrivilegedPort() {

    try {

        SystemClock clock;
        Storage storage(":memory:");
        storage.initializeSchema();
        Logger logger(storage, clock, std::cerr);
        Authenticator authenticator(storage, clock, logger);
        Inventory inventory(storage, logger);
        Server s(80, inventory, authenticator, logger, storage);

        TEST_FAIL_MESSAGE("Expected std::runtime_error, but no exception was thrown.");

    } catch (const std::runtime_error &e) {
        cout << "\n--- DEBUG INFO ---\n";
        cout << "Exception message (e.what()): [" << e.what() << "]\n";
        cout << "--- END DEBUG ---\n";
        TEST_ASSERT_NOT_NULL(strstr(e.what(), "bind"));

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected std::runtime_error, but a different exception was thrown.");
    }
}