#include "clock.hpp"
#include "logger.hpp"
#include "storage.hpp"
#include "unity.h"
#include <iostream>
#include <sstream>

/**
 * @brief Mock clock for testing. Allows setting a specific time.
 */
class MockClock : public IClock {
  public:
    std::time_t now() const override {
        return m_time;
    }
    void set_time(std::time_t new_time) {
        m_time = new_time;
    }

  private:
    std::time_t m_time = 0;
};

void testSystemLog() {
    std::string hostname = "Warehouse-A";
    Storage storage(":memory:");
    MockClock clock;
    clock.set_time(5000);
    Logger logger(storage, clock, std::cerr);
    storage.initializeSchema();

    LogLevel level = LogLevel::INFO;
    std::string component = "System";
    std::string message = "Server is shuting down...";
    logger.log(level, component, message);

    try {

        SQLite::Statement query(storage.getDb(), "SELECT level, message, client_id FROM logs WHERE component = ?");
        query.bind(1, component);

        TEST_ASSERT_TRUE(query.executeStep());

        TEST_ASSERT_EQUAL_STRING("INFO", query.getColumn("level"));
        TEST_ASSERT_EQUAL_STRING(message.c_str(), query.getColumn("message"));

        TEST_ASSERT_TRUE(query.getColumn("client_id").isNull());

    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

void testComponentLogWithClientID() {
    Storage storage(":memory:");
    MockClock clock;
    clock.set_time(5000);
    Logger logger(storage, clock, std::cerr);
    storage.initializeSchema();

    LogLevel level = LogLevel::INFO;
    std::string component = "Authenticator";
    std::string message = "Login successful for user";
    std::optional<std::string> clientId = "warehouse-A";
    storage.createUser("warehouse-A", "pass123");

    logger.log(level, component, message, clientId);

    try {

        SQLite::Statement query(storage.getDb(), "SELECT level, message, component FROM logs WHERE client_id = ?");
        query.bind(1, clientId.value());

        TEST_ASSERT_TRUE(query.executeStep());

        TEST_ASSERT_EQUAL_STRING("INFO", query.getColumn("level"));
        TEST_ASSERT_EQUAL_STRING(message.c_str(), query.getColumn("message"));

        TEST_ASSERT_EQUAL_STRING(component.c_str(), query.getColumn("component"));

    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE(e.what());
    }
}

void testLogFailsGracefullyWithInvalidClientId() {
    Storage storage(":memory:");
    MockClock clock;
    std::stringstream errorCaptureStream;
    Logger logger(storage, clock, errorCaptureStream);
    storage.initializeSchema();

    LogLevel level = LogLevel::INFO;
    std::string component = "Authenticator";
    std::string message = "Login successful for user: ";
    std::optional<std::string> clientId = "Not a valid client";

    logger.log(level, component, message, clientId);
    std::string errorOutput = errorCaptureStream.str();
    TEST_ASSERT_NOT_NULL(strstr(errorOutput.c_str(), "CRITICAL LOGGER FAILURE"));
    TEST_ASSERT_NOT_NULL(strstr(errorOutput.c_str(), "FOREIGN KEY constraint failed"));
}

void testLogLevel() {

    std::string info_str = Logger::levelToString(LogLevel::INFO);
    TEST_ASSERT_EQUAL_STRING("INFO", info_str.c_str());

    std::string warning_str = Logger::levelToString(LogLevel::WARNING);
    TEST_ASSERT_EQUAL_STRING("WARNING", warning_str.c_str());

    std::string error_str = Logger::levelToString(LogLevel::ERROR);
    TEST_ASSERT_EQUAL_STRING("ERROR", error_str.c_str());
}