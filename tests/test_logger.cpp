#include "clock.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "storage.hpp"
#include "test_helper.hpp"
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

    Config config = createDummyConfig();
    std::string hostname = "Warehouse-A";
    Storage storage(":memory:");
    MockClock clock;
    clock.set_time(5000);
    Logger logger(storage, clock, std::cerr, config);
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
    Config config = createDummyConfig();
    Storage storage(":memory:");
    MockClock clock;
    clock.set_time(5000);
    Logger logger(storage, clock, std::cerr, config);
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
    Config config = createDummyConfig();
    Storage storage(":memory:");
    MockClock clock;
    std::stringstream errorCaptureStream;
    Logger logger(storage, clock, errorCaptureStream, config);
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

void testOpenLogFileSuccess() {
    Config config = createDummyConfig();
    Storage storage(":memory:");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, config);
    bool result = logger.openLogFile("./var/logs/server_test.log");
    TEST_ASSERT_TRUE(result);

    // Cleanup
    logger.closeLogFile();
    std::filesystem::remove("./var/logs/server_test.log");
}

void testOpenLogFileFailure() {
    Config config = createDummyConfig();
    Storage storage(":memory:");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, config);
    bool result = logger.openLogFile("/root/forbidden_log.log"); // Assuming no permissions
    TEST_ASSERT_FALSE(result);
}

void testLoggerWritesToFile() {
    Config config = createDummyConfig();
    Storage storage(":memory:");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, config);

    std::string testLogPath = "./var/logs/test_logger.log";
    logger.openLogFile(testLogPath);

    logger.log(LogLevel::INFO, "TestComponent", "Test message", std::nullopt);

    std::ifstream logFile(testLogPath);
    std::string line;
    std::getline(logFile, line);

    TEST_ASSERT_NOT_NULL(strstr(line.c_str(), "Test message"));
    logFile.close();
    std::remove(testLogPath.c_str());
}

void testLoggerCloseLogFile() {
    Config config = createDummyConfig();
    Storage storage(":memory:");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, config);

    std::string testLogPath = "./var/logs/test_logger_close.log";
    logger.openLogFile(testLogPath);

    logger.closeLogFile();

    TEST_ASSERT_FALSE(logger.isFileEnabled());
    TEST_ASSERT_FALSE(logger.isLogFileOpen());

    std::filesystem::remove(testLogPath);
}

void testLoggerLogRotation() {
    Config config = createDummyConfig();
    Storage storage(":memory:");
    MockClock clock;
    clock.set_time(123456789);
    Logger logger(storage, clock, std::cerr, config);

    std::string logPath = "./var/logs/test_rotation.log";
    logger.openLogFile(logPath);
    logger.log(LogLevel::INFO, "TestComponent", "Rotating log test", std::nullopt);

    logger.logRotation();

    std::string expectedGz = "./var/logs/server_123456789.log.gz";
    TEST_ASSERT_TRUE(std::filesystem::exists(expectedGz));
    TEST_ASSERT_TRUE(std::filesystem::exists(logPath));

    // The original file should exist as a new empty log
    std::ifstream newLogFile(logPath);
    TEST_ASSERT_TRUE(newLogFile.is_open());

    // Cleanup
    newLogFile.close();
    std::filesystem::remove(logPath);
    std::filesystem::remove(expectedGz);
}

void testCompressFileGzipFailsOnInvalidPath() {
    Config config = createDummyConfig();
    Storage storage(":memory:");
    SystemClock clock;
    Logger logger(storage, clock, std::cerr, config);

    // source file does not exist
    bool result = logger.compressFileGzip("./var/logs/no_such_file.log", "./var/logs/should_not_exist.gz");
    TEST_ASSERT_FALSE(result);

    // Destination without permissions (optional)
    bool result2 = logger.compressFileGzip("./var/logs/test_rotation.log", "/root/forbidden.gz");
    TEST_ASSERT_FALSE(result2);
}

void testShouldRotateTrue() {
    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 80
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"
                queue_size: 10
                unix_socket_path: "/tmp/server_test.sock"
            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/test_should_rotate.log"
            )");

    Storage storage(":memory:");
    SystemClock clock;
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};
    Config config(args);
    Logger logger(storage, clock, std::cerr, config);

    std::string logPath = "./var/logs/test_should_rotate.log";
    // Create a 10 MB file
    std::ofstream out(logPath, std::ios::binary);
    out.seekp((config.getMaxLogSize() * 1024 * 1024) - 1);
    out.write("", 1);
    out.close();

    TEST_ASSERT_TRUE(logger.shouldRotate());

    // Cleanup
    std::filesystem::remove(logPath);
}