#include "client.h"
#include "client_context.h"
#include "input_handler.h"
#include "session_handler.h"
#include "transport.h"
#include "unity.h"
#include <string.h>

void test_execute_client_action_error() {
    ClientContext context;
    client_context_init(&context);
    char buffer[BUFFER_SIZE] = "some input";
    transaction_result res = execute_client_action(&context, INPUT_ACTION_ERROR, buffer, tcp_recv, tcp_send);
    TEST_ASSERT_EQUAL(TRANSACTION_SUCCESS, res);
}

void test_execute_client_action_continue() {
    ClientContext context;
    client_context_init(&context);
    char buffer[BUFFER_SIZE] = "some input";
    transaction_result res = execute_client_action(&context, INPUT_ACTION_CONTINUE, buffer, tcp_recv, tcp_send);
    TEST_ASSERT_EQUAL(TRANSACTION_SUCCESS, res);
}

void test_execute_client_action_quit_error() {
    ClientContext context;
    client_context_init(&context);
    char buffer[BUFFER_SIZE] = "end";
    transaction_result res = execute_client_action(&context, INPUT_ACTION_QUIT, buffer, tcp_recv, tcp_send);
    TEST_ASSERT_EQUAL(TRANSACTION_ERROR, res);
}

void test_execute_client_action_send_error() {
    ClientContext context;
    client_context_init(&context);
    char buffer[BUFFER_SIZE] = "login user1 123";
    transaction_result res = execute_client_action(&context, INPUT_ACTION_SEND, buffer, tcp_recv, tcp_send);
    TEST_ASSERT_EQUAL(TRANSACTION_ERROR, res);
}

void test_execute_client_action_syntax_error() {
    ClientContext context;
    client_context_init(&context);
    char buffer[BUFFER_SIZE] = "login";
    transaction_result res = execute_client_action(&context, INPUT_ACTION_SEND, buffer, tcp_recv, tcp_send);
    TEST_ASSERT_EQUAL(TRANSACTION_SUCCESS, res);
}

void test_start_communication_fails_uninitialized_socket() {
    ClientContext context;
    client_context_init(&context);
    context.tcp_socket = -1; // Simulate an uninitialized socket
    int res = start_communication(&context, tcp_recv, tcp_send);
    TEST_ASSERT_EQUAL(-1, res); // Expect failure due to uninitialized socket
}

void test_launch_dashboard_fails_without_client_id() {
    ClientContext context;
    client_context_init(&context);
    context.tcp_socket = 1; // Simulate a valid socket
    bool res = launch_dashboard(&context);
    TEST_ASSERT_FALSE(res); // Expect failure due to missing client ID
}
