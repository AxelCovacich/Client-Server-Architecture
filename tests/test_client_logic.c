#include "client.h"
#include "transport.h"
#include "unity.h"
#include <netinet/in.h> // For sockaddr_in
#include <stdbool.h>
#include <string.h>
#include <string.h> // For memcmp

/**
 * @brief Tests that setup_and_connect fails when given a non-numeric port.
 */
void test_setup_and_connect_fails_with_invalid_port_string() {
    client_config config = {.host = "localhost", .port_tcp = "invalid_port", .port_udp = "8001"};
    ClientContext context = {0};

    bool result = setup_and_connect(&context, config, "tcp");

    TEST_ASSERT_FALSE(result);
}

void test_setup_and_connect_fails_with_invalid_host() {
    client_config config = {.host = "no_such_host", .port_tcp = "8000", .port_udp = "8001"};
    ClientContext context = {0};

    bool result = setup_and_connect(&context, config, "tcp");

    TEST_ASSERT_FALSE(result);
}

void test_setup_and_connect_fails_with_invalid_udp_port_string() {
    client_config config = {.host = "localhost", .port_tcp = "8000", .port_udp = "invalid_port"};
    ClientContext context = {0};

    bool result = setup_and_connect(&context, config, "udp");

    TEST_ASSERT_FALSE(result);
}

void test_initialize_udp_peer_address_copies_data() {

    // Create a fake IPv4 address structure to use as input.
    struct sockaddr_in fake_address = {0};
    fake_address.sin_family = AF_INET;
    fake_address.sin_port = htons(8080);

    initialize_udp_peer_address((const struct sockaddr *)&fake_address, sizeof(fake_address));

    const struct sockaddr_storage *internal_address = get_peer_address_for_test();

    TEST_ASSERT_EQUAL_INT(0, memcmp(&fake_address, internal_address, sizeof(fake_address)));
}