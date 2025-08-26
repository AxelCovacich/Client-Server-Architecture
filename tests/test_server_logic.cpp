#include "config.hpp"
#include "server.hpp"
#include "sessionManager.hpp"
#include "test_helper.hpp"
#include "trafficReporter.hpp"
#include "unity.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

using namespace std;

void testServerConstructorFailsOnPrivilegedPort() {

    try {
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

            logger:
                max_log_size_mb: 10
                log_path: "./var/logs/server.log"
            )");

        const std::vector<std::string> args = {"./server", "./temp_config.yaml"};
        Config config(args);
        std::cout << "max_log_size_mb: " << config.getMaxLogSize() << "\n";
        SystemClock clock;
        Storage storage(":memory:");
        Logger logger(storage, clock, std::cerr, config);
        TrafficReporter trafficReporter;
        Server s(config, clock, storage, logger, trafficReporter);

        TEST_FAIL_MESSAGE("Expected std::runtime_error, but no exception was thrown.");

    } catch (const std::runtime_error &e) {
        cout << "\n--- DEBUG INFO ---\n";
        cout << "Exception message (e.what()): [" << e.what() << "]\n";
        cout << "--- END DEBUG ---\n";
        TEST_ASSERT_NOT_NULL(strstr(e.what(), "bind"));

    } catch (...) {

        TEST_FAIL_MESSAGE("Expected std::runtime_error, but a different exception was thrown.");
    }
}

void testGetClientIpIpv4() {
    struct sockaddr_in addr4 = {};
    addr4.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.0.1", &addr4.sin_addr);

    struct sockaddr_storage address = {};
    memcpy(&address, &addr4, sizeof(addr4));

    std::string ip = Server::getClientIP(address);
    TEST_ASSERT_EQUAL_STRING("192.168.0.1", ip.c_str());
}

void testGetClientIpIpv6() {
    struct sockaddr_in6 addr6 = {};
    addr6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "2001:db8::1", &addr6.sin6_addr);

    struct sockaddr_storage address = {};
    memcpy(&address, &addr6, sizeof(addr6));

    std::string ip = Server::getClientIP(address);
    TEST_ASSERT_EQUAL_STRING("2001:db8::1", ip.c_str());
}

void testGetClientIpInvalid() {
    struct sockaddr_storage address = {};
    std::string ip = Server::getClientIP(address);
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", ip.c_str());
}