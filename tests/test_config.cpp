// tests/test_config.cpp

#include "config.hpp"
#include "test_helper.hpp"
#include "unity.h"
#include <cstdio>

void testConfigLoadsPortFromYaml() {

    createTempYamlFile(R"(
server:
  port: 9999
database:
  path: "./db_from_test.sqlite3"
security:
  unlock_secret_phrase: "test_phrase"
)");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    TEST_ASSERT_EQUAL_INT(9999, config.getPort());

    remove("./temp_config.yaml");
}

void testConfigPrioritizesCliArgumentOverYaml() {

    createTempYamlFile(R"(
server:
  port: 9999
database:
  path: "./db_from_test.sqlite3"
security:
  unlock_secret_phrase: "test_phrase"
)");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml", "8080"};

    Config config(args);

    TEST_ASSERT_EQUAL_INT(8080, config.getPort());

    remove("./temp_config.yaml");
}
/*
/**
 * @brief Tests that Config prioritizes an environment variable over the YAML file.
 /
void testConfigPrioritizesEnvVariableOverYaml() {

    createTempYamlFile("server:\n  port: 9999");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    //ask for port stored on getenv
    TEST_ASSERT_EQUAL_INT(7777, config.getPort());

    remove("./temp_config.yaml");
}

/**
 * @brief Tests that Config correctly loads the database path from the YAML file.
 */
void testConfigLoadsdbPathFromYaml() {

    createTempYamlFile(R"(
server:
  port: 9999
database:
  path: "./test/db/from_yaml.sqlite3"
security:
  unlock_secret_phrase: "test_phrase"
)");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    TEST_ASSERT_EQUAL_STRING("./test/db/from_yaml.sqlite3", config.getDbPath().c_str());

    remove("./temp_config.yaml");
}

/**
 * @brief Tests that Config prioritizes the DB_PATH environment variable over the YAML file.

void test_config_prioritizes_env_variable_for_db_path() {

    createTempYamlFile(R"(
                            database:
                            path: "./path_from_yaml.sqlite3"
                            server:
                            port: 8080
                            )");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    TEST_ASSERT_EQUAL_STRING("/path/from/env", config.getDbPath().c_str());

    remove("./temp_config.yaml");
}
*\

// TODO: The test for environment variable priority is currently handled by manual
// verification, as it requires a specific execution environment.
// To test automatically in the future, the test runner would need to be
// invoked with the environment variables set, e.g.:
// $ SERVER_PORT=7777 ctest
*/

void testConfigSecretPhraseFromYaml() {

    createTempYamlFile(R"(
server:
  port: 9999
database:
  path: "./db_from_test.sqlite3"
security:
  unlock_secret_phrase: "test_phrase"
)");
    const std::vector<std::string> args = {"./server", "./temp_config.yaml"};

    Config config(args);

    TEST_ASSERT_EQUAL_STRING("test_phrase", config.getSecretPhrase().c_str());

    remove("./temp_config.yaml");
}