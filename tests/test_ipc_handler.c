#include "client_context.h"
#include "ipc_handler.h"
#include "unity.h"
#include <string.h>

void test_ipc_init(void) {
    ClientContext context;
    TEST_ASSERT_TRUE(ipc_init(&context));
    TEST_ASSERT_NOT_EQUAL((mqd_t)-1, context.ipc_queue);
    mq_unlink(IPC_QUEUE_NAME);
}

void test_ipc_priority_check(void) {
    const char *keepalive_message = "{\"category\":\"keepalive\",\"message\":\"some message\"}";
    const char *alert_message = "{\"category\":\"alert\",\"message\":\"some message\"}";
    const char *exit_message = "{\"category\":\"exit\",\"message\":\"some message\"}";
    const char *unknown_message = "{\"category\":\"unknown\",\"message\":\"some message\"}";

    TEST_ASSERT_EQUAL(2, priority_check("{\"category\":\"alert\"}"));
    TEST_ASSERT_EQUAL(1, priority_check("{\"category\":\"keepalive\"}"));
    TEST_ASSERT_EQUAL(3, priority_check("{\"category\":\"exit\"}"));
    TEST_ASSERT_EQUAL(0, priority_check("{\"category\":\"unknown\"}"));
}

void test_ipc_priority_check_invalid_json(void) {
    const char *invalid_json = "{\"message\":\"some message\"}"; // Missing category
    TEST_ASSERT_EQUAL(-1, priority_check(invalid_json));
}

void test_ipc_send_message_long_message_fails(void) {
    ClientContext context;
    ipc_init(&context);

    char long_message[MAX_MSG_SIZE + 1];
    memset(long_message, 'A', MAX_MSG_SIZE);
    long_message[MAX_MSG_SIZE] = '\0'; // Ensure it's a string

    TEST_ASSERT_FALSE(ipc_send_message(&context, long_message));
    mq_unlink(IPC_QUEUE_NAME);
}

void test_ipc_send_message_not_initialized_fails(void) {
    ClientContext context;
    context.ipc_queue = (mqd_t)-1; // Simulate uninitialized queue

    TEST_ASSERT_FALSE(ipc_send_message(&context, "Test message"));
}

void test_ipc_send_message_success(void) {
    ClientContext context;
    ipc_init(&context);

    const char *message = "{\"category\":\"keepalive\",\"message\":\"Test message\"}";
    TEST_ASSERT_TRUE(ipc_send_message(&context, message));

    mq_unlink(IPC_QUEUE_NAME);
}

void test_ipc_exit(void) {
    ClientContext context;
    ipc_init(&context);

    TEST_ASSERT_TRUE(ipc_exit(&context));
    mq_unlink(IPC_QUEUE_NAME);
}