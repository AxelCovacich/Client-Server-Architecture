#include "client.h"
#include "unity.h"
#include <stdbool.h>
#include <string.h>

/**
 * @brief Tests that setup_and_connect fails when given a non-numeric port.
 */
void test_setup_and_connect_fails_with_invalid_port_string() {

    int result_fd = setup_and_connect("localhost", "invalid port");

    TEST_ASSERT_EQUAL_INT(-1, result_fd);
}