#include "argsParser.hpp"
#include "unity.h"

void testParserFailsWithInsufficientArgs() {
    const char *argv[] = {"./server"};
    int argc = 1;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);
    TEST_ASSERT_FALSE(result.has_value());
}

void testParserFailsWithNonNumericPort() {
    const char *argv[] = {"./server", "abc", "path"};
    int argc = 3;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);
    TEST_ASSERT_FALSE(result.has_value());
}

void testParserSucceedsWithCorrectArgs() {
    const char *argv[] = {"./server", "8080", "path_to_db"};
    int argc = 3;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);
    TEST_ASSERT_TRUE(result.has_value());
    TEST_ASSERT_EQUAL_INT(8080, result->port);
    TEST_ASSERT_EQUAL_STRING("path_to_db", result->dbPath.c_str());
}

/**
 * @brief Tests that the parser fails when the port number is out of the valid range.
 */
void testParserInvalidPortNumber() {
    const char *argv[] = {"./server", "80800", "path"};
    int argc = 3;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);
    TEST_ASSERT_FALSE(result.has_value());
}

/**
 * @brief Tests that the parser fails when the port number is zero or negative.
 */
void testParserZeroNumberPort() {
    const char *argv[] = {"./server", "0", "path"};
    int argc = 3;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);

    TEST_ASSERT_FALSE(result.has_value());
}

void testParserFailsWithExcessiveArgs() {

    const char *argv[] = {"./server", "8080", "path", "extra_argument"};
    int argc = 4;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);

    TEST_ASSERT_FALSE(result.has_value());
}