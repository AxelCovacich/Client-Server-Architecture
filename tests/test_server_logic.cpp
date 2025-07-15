#include "commandProcessor.hpp"
#include "server.hpp"
#include "unity.h"
#include <cstring>
#include <stdexcept>
#include <string>

#include <iostream>

using namespace std;

void testProcessCommandStatus() {
    commandProcessor::commandResult result = commandProcessor::processCommand("status");
    TEST_ASSERT_EQUAL_STRING("STATUS: OK\n", result.first.c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testProcessCommandEnd() {
    commandProcessor::commandResult result = commandProcessor::processCommand("end");
    TEST_ASSERT_EQUAL_STRING("ACK: Disconnect command received.\n", result.first.c_str());
    TEST_ASSERT_FALSE(result.second);
}

void testProcessCommandUnknown() {
    commandProcessor::commandResult result = commandProcessor::processCommand("any");
    TEST_ASSERT_EQUAL_STRING("ACK: Message received.\n", result.first.c_str());
    TEST_ASSERT_TRUE(result.second);
}

void testServerConstructorFailsOnPrivilegedPort() {

    try {

        Server s(80);
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