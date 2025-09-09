#include "config.hpp"
#include "eventQueue.hpp"
#include "storage.hpp"
#include "test_helper.hpp"
#include "unity.h"
#include <iostream>
#include <string>

void testEventQueuePushLimitErasesOld() {
    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    EventQueue eventQueue(3, logger);
    eventQueue.pushEvent("client1", Event{EventType::NOTIFICATION, "Event 1"});
    eventQueue.pushEvent("client1", Event{EventType::NOTIFICATION, "Event 2"});
    eventQueue.pushEvent("client1", Event{EventType::NOTIFICATION, "Event 3"});
    eventQueue.pushEvent("client1", Event{EventType::NOTIFICATION, "Event 4"}); // This should evict "Event 1"

    Event event;
    eventQueue.popEvent("client1", event);
    TEST_ASSERT_EQUAL_STRING("Event 2", event.message.c_str()); // The first event should now be "Event 2"
}

void testEventQueuePopEmpty() {
    Storage storage(":memory:");
    storage.initializeSchema();
    SystemClock clock;
    Config dummyConfig = createDummyConfig();
    Logger logger(storage, clock, std::cerr, dummyConfig);
    EventQueue eventQueue(3, logger);
    Event event;
    eventQueue.pushEvent("client1", Event{EventType::NOTIFICATION, "Event 1"});
    eventQueue.popEvent("client1", event); // Remove the only event to make it empty
    bool result = eventQueue.popEvent("client1", event);
    TEST_ASSERT_FALSE(result); // Should return false as there are no events
}