// tests/test_config.cpp

#include "config.hpp"
#include "test_helper.hpp"
#include "unity.h"
#include <cstdio>

void testConfigLoadsPortFromYaml() {

    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 9999
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    TEST_ASSERT_EQUAL_INT(9999, config.getTcpPort());

    remove("./temp_config.yaml");
}

void testConfigPrioritizesCliArgumentOverYaml() {

    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 9999
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml", "8080"};

    Config config(args);

    TEST_ASSERT_EQUAL_INT(8080, config.getTcpPort());

    remove("./temp_config.yaml");
}
/**
 * @brief Tests that Config prioritizes an environment variable over the YAML file.
 */
void testConfigPrioritizesEnvVariableOverYaml() {

    setenv("SERVER_PORT", "7777", 1);
    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 9999
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    // ask for port stored on getenv
    TEST_ASSERT_EQUAL_INT(7777, config.getTcpPort());
    unsetenv("SERVER_PORT"); // Clean up environment variable
    remove("./temp_config.yaml");
}

/**
 * @brief Tests that Config correctly loads the database path from the YAML file.
 */
void testConfigLoadsdbPathFromYaml() {

    createTempYamlFile(R"(
            database:
                path: "./test/db/from_yaml.sqlite3"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 80
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    TEST_ASSERT_EQUAL_STRING("./test/db/from_yaml.sqlite3", config.getDbPath().c_str());

    remove("./temp_config.yaml");
}

void testConfigSecretPhraseFromYaml() {

    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test_phrase"
                block_time_seconds: 900

            server:
                port: 80
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    TEST_ASSERT_EQUAL_STRING("test_phrase", config.getSecretPhrase().c_str());

    remove("./temp_config.yaml");
}

void testConfigPrioritizesCliArgumentsTcpAndUdp() {
    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 9999
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml", "8080", "8081"};

    Config config(args);

    TEST_ASSERT_EQUAL_INT(8080, config.getTcpPort());
    TEST_ASSERT_EQUAL_INT(8081, config.getUdpPort());

    remove("./temp_config.yaml");
}

void testConfigFailsOnInvalidPortInYaml() {
    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: -1
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    try {
        Config config(args);
    } catch (const std::runtime_error &e) {
        TEST_ASSERT_EQUAL_STRING("Port in config file out of valid range", e.what());
    }
    remove("./temp_config.yaml");
}

void testConfigFailsOnInvalidEnvPort() {
    setenv("SERVER_PORT", "not_a_number", 1);
    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 9999
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    try {
        Config config(args);
        TEST_FAIL_MESSAGE("Expected std::runtime_error but no exception was thrown.");

    } catch (const std::runtime_error &e) {
        TEST_ASSERT_EQUAL_STRING("Environment variable SERVER_PORT is not a valid number", e.what());
    } catch (...) {
        TEST_FAIL_MESSAGE("Expected runtime_error but a different exception was thrown.");
    }
    unsetenv("SERVER_PORT");
    remove("./temp_config.yaml");
}

void testConfigCliArgumentHasPriorityOverEnv() {
    setenv("SERVER_PORT", "7777", 1);
    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 5555
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml", "8080"};

    Config config(args);

    TEST_ASSERT_EQUAL_INT(8080, config.getTcpPort());

    unsetenv("SERVER_PORT");
    remove("./temp_config.yaml");
}

void testConfigFailsOnMissingDbPathInYaml() {
    createTempYamlFile(R"(
            security:
                unlock_secret_phrase: "test"
                block_time_seconds: 900

            server:
                port: 80
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    try {
        Config config(args);
        TEST_FAIL_MESSAGE("Expected std::runtime_error but no exception was thrown.");
    } catch (const std::runtime_error &e) {
        TEST_ASSERT_EQUAL_STRING("Database path is not set in config file", e.what());
    } catch (...) {
        TEST_FAIL_MESSAGE("Expected runtime_error but a different exception was thrown.");
    }

    remove("./temp_config.yaml");
}

void testConfigFailsOnMissingSecretPhraseInYaml() {
    createTempYamlFile(R"(
            database:
                path: ":memory:"

            security:
                block_time_seconds: 900

            server:
                port: 80
                max_clients: 10
                max_unix_connections: 5
                metric_host_port: "localhost:8081"

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");

    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    try {
        Config config(args);
        TEST_FAIL_MESSAGE("Expected std::runtime_error but no exception was thrown.");
    } catch (const std::runtime_error &e) {
        TEST_ASSERT_EQUAL_STRING("Secret phrase is not set in config file", e.what());
    } catch (...) {
        TEST_FAIL_MESSAGE("Expected runtime_error but a different exception was thrown.");
    }

    remove("./temp_config.yaml");
}

void testConfigLoadsFromDefaultWhenNoArgs() {
    const std::vector<std::string> args = {"./server"};
    try {
        Config config(args);
        // Basic sanity checks to ensure the config loaded
        TEST_ASSERT_TRUE(config.getTcpPort() > 0);
        TEST_ASSERT_FALSE(config.getDbPath().empty());
        TEST_ASSERT_FALSE(config.getSecretPhrase().empty());
    } catch (const std::exception &e) {
        TEST_FAIL_MESSAGE("Expected successful load from default config, but exception thrown.");
    }
}
