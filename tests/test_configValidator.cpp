// test_config_validator.cpp
#include "configValidator.hpp"
#include "unity.h"
#include <cstdio>  // remove()
#include <fstream> // create
#include <iostream>
#include <vector>

void testValidatorFailsOnExcessiveArgs() {

    const std::vector<std::string> args = {"./server", "./config.yaml", "8080", "extra"};

    bool isValid = ConfigValidator::validateArguments(args);

    TEST_ASSERT_FALSE(isValid);
}

void testValidatorFailsOnNonNumericPort() {

    const char *tempFilePath = "./temp_config_for_test.yaml";
    std::ofstream tempFile(tempFilePath);
    const std::vector<std::string> args = {"./server", tempFilePath, "abc"};

    bool isValid = ConfigValidator::validateArguments(args);

    TEST_ASSERT_FALSE(isValid);
}

void testValidatorFailsWithInsufficientArgs() {
    const std::vector<std::string> args = {"./server"};

    bool isValid = ConfigValidator::validateArguments(args);
    TEST_ASSERT_FALSE(isValid);
}

void testValidatorSucceedsWithCorrectArgs() {

    const char *tempFilePath = "./temp_config_for_test.yaml";
    std::ofstream tempFile(tempFilePath);
    const std::vector<std::string> args = {"./server", tempFilePath, "7000"};

    bool isValid = ConfigValidator::validateArguments(args);
    TEST_ASSERT_TRUE(isValid);
}

/**
 * @brief Tests that the parser fails when the port number is out of the valid range.
 */
void testValidatorrInvalidPortNumber() {
    const char *tempFilePath = "./temp_config_for_test.yaml";
    std::ofstream tempFile(tempFilePath);
    const std::vector<std::string> args = {"./server", tempFilePath, "80800"};

    bool isValid = ConfigValidator::validateArguments(args);

    TEST_ASSERT_FALSE(isValid);
}

/**
 * @brief Tests that the parser fails when the port number is zero or negative.
 */
void testValidatorZeroNumberPort() {

    const char *tempFilePath = "./temp_config_for_test.yaml";
    std::ofstream tempFile(tempFilePath);
    const std::vector<std::string> args = {"./server", tempFilePath, "0"};

    bool isValid = ConfigValidator::validateArguments(args);

    TEST_ASSERT_FALSE(isValid);
}

void testValidatesFalseFileNotFound() {

    const std::vector<std::string> args = {"./server", "./some/path", "8080"};

    bool isValid = ConfigValidator::validateArguments(args);

    TEST_ASSERT_FALSE(isValid);
}
