#include "trafficReporter.hpp"
#include "unity.h"

void testTrafficReporterCounters() {
    TrafficReporter trafficReporter;

    trafficReporter.incrementReconnection("tcp", "rx");

    trafficReporter.incrementMessage("tcp", "rx");
    trafficReporter.incrementMessage("tcp", "rx");
    trafficReporter.incrementMessage("tcp", "rx");

    trafficReporter.incrementMessage("tcp", "tx");

    trafficReporter.incrementMessage("udp", "rx");
    trafficReporter.incrementMessage("udp", "rx");
    trafficReporter.incrementMessage("udp", "rx");

    trafficReporter.incrementMessage("udp", "tx");

    trafficReporter.incrementMessage("icp", "rx");
    trafficReporter.incrementMessage("icp", "rx");
    trafficReporter.incrementMessage("icp", "rx");

    trafficReporter.incrementMessage("icp", "tx");
    trafficReporter.incrementMessage("icp", "tx");

    trafficReporter.incrementError("tcp", "tx");
    trafficReporter.incrementError("tcp", "tx");

    trafficReporter.incrementError("tcp", "rx");
    trafficReporter.incrementError("tcp", "rx");

    trafficReporter.incrementError("udp", "rx");
    trafficReporter.incrementError("udp", "rx");

    trafficReporter.incrementError("udp", "tx");
    trafficReporter.incrementError("udp", "tx");

    trafficReporter.incrementError("icp", "rx");
    trafficReporter.incrementError("icp", "rx");

    trafficReporter.incrementError("icp", "tx");

    TEST_ASSERT_EQUAL_INT(1, trafficReporter.getReconnectionCount("tcp", "rx"));
    TEST_ASSERT_EQUAL_INT(2, trafficReporter.getErrorCount("tcp", "rx"));
    TEST_ASSERT_EQUAL_INT(3, trafficReporter.getMessageCount("tcp", "rx"));
    TEST_ASSERT_EQUAL_INT(1, trafficReporter.getMessageCount("tcp", "tx"));
    TEST_ASSERT_EQUAL_INT(3, trafficReporter.getMessageCount("udp", "rx"));
    TEST_ASSERT_EQUAL_INT(1, trafficReporter.getMessageCount("udp", "tx"));
    TEST_ASSERT_EQUAL_INT(3, trafficReporter.getMessageCount("icp", "rx"));
    TEST_ASSERT_EQUAL_INT(2, trafficReporter.getMessageCount("icp", "tx"));
    TEST_ASSERT_EQUAL_INT(2, trafficReporter.getErrorCount("tcp", "tx"));
    TEST_ASSERT_EQUAL_INT(2, trafficReporter.getErrorCount("udp", "tx"));
    TEST_ASSERT_EQUAL_INT(2, trafficReporter.getErrorCount("udp", "rx"));
    TEST_ASSERT_EQUAL_INT(2, trafficReporter.getErrorCount("icp", "rx"));
    TEST_ASSERT_EQUAL_INT(1, trafficReporter.getErrorCount("icp", "tx"));
}