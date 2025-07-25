#include "argsParser.hpp"
#include "unity.h"

void testParserFailsWithInsufficientArgs() {
    char *argv[] = {"./server"};
    int argc = 1;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);
    TEST_ASSERT_FALSE(result.has_value());
}

void testParserFailsWithNonNumericPort() {
    char *argv[] = {"./server", "abc"};
    int argc = 2;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);
    TEST_ASSERT_FALSE(result.has_value());
}

void testParserSucceedsWithCorrectArgs() {
    char *argv[] = {"./server", "8080"};
    int argc = 2;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);
    TEST_ASSERT_TRUE(result.has_value());
    TEST_ASSERT_EQUAL_INT(8080, *result);
}

/**
 * @brief Tests that the parser fails when the port number is out of the valid range.
 */
void testParserInvalidPortNumber() {
    char *argv[] = {"./server", "80800"};
    int argc = 2;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);
    TEST_ASSERT_FALSE(result.has_value());
}

/**
 * @brief Tests that the parser fails when the port number is zero or negative.
 */
void testParserZeroNumberPort() {
    char *argv[] = {"./server", "0"};
    int argc = 2;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);

    TEST_ASSERT_FALSE(result.has_value());
}

void testParserFailsWithExcessiveArgs() {

    char *argv[] = {"./server", "8080", "extra_argument"};
    int argc = 3;

    const std::vector<std::string> args(argv, argv + argc);
    auto result = ArgsParser::parseArguments(args);

    // 3. Assert: Verificamos que el resultado es un optional vacío,
    //    lo que indica un fallo en el parseo.
    TEST_ASSERT_FALSE(result.has_value());
}