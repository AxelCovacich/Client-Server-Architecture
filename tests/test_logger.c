#include "logger.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

void test_logger_init_success() {
    // test path is valid
    int res = logger_init("../../var/logs/client.log");
    if (res != 0) {
        perror("Logger initialization failed");
    }
    TEST_ASSERT_EQUAL(0, res);
    logger_close();
}

void test_logger_init_fail() {
    // invalid path to trigger failure
    int res = logger_init("/some/invalid/path");
    TEST_ASSERT_EQUAL(-1, res);
    logger_close();
}

void test_logger_log_writes_to_file() {
    const char *log_path = "../../var/logs/test_client.log";
    logger_init(log_path);

    logger_log("TestComponent", INFO, "Test message");
    logger_close();

    FILE *f = fopen(log_path, "r");
    TEST_ASSERT_NOT_NULL(f);

    char line[256];
    TEST_ASSERT_NOT_NULL(fgets(line, sizeof(line), f));
    fclose(f);

    TEST_ASSERT_NOT_NULL(strstr(line, "[20")); // 20XX
    TEST_ASSERT_TRUE(strlen(line) > 20);
    TEST_ASSERT_NOT_NULL(strstr(line, "TestComponent"));
    TEST_ASSERT_NOT_NULL(strstr(line, "INFO"));
    TEST_ASSERT_NOT_NULL(strstr(line, "Test message"));
}

void test_level_to_string() {
    TEST_ASSERT_EQUAL_STRING("DEBUG", log_level_to_string(DEBUG));
    TEST_ASSERT_EQUAL_STRING("INFO", log_level_to_string(INFO));
    TEST_ASSERT_EQUAL_STRING("WARNING", log_level_to_string(WARNING));
    TEST_ASSERT_EQUAL_STRING("ERROR", log_level_to_string(ERROR));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", log_level_to_string((log_level)999)); // Invalid level
}