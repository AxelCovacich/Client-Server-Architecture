#include "client.h"
#include "client_context.h"
#include "input_handler.h"
#include "session_handler.h"
#include "transport.h"
#include "unity.h"
#include <string.h>

ssize_t mock_send_success(int sockfd, const void *buf, size_t len, int flags) {
    return len;
}

ssize_t mock_recv_success(int sockfd, void *buf, size_t len, int flags) {
    const char *response = "{\"status\":\"ok\"}";
    size_t resp_len = strlen(response);
    memcpy(buf, response, resp_len);
    return resp_len;
}

ssize_t mock_recv_closed(int sockfd, void *buf, size_t len, int flags) {
    return 0;
}

ssize_t mock_send_error(int sockfd, const void *buf, size_t len, int flags) {
    return -1;
}
char *mock_input_end(char *buf, int size, FILE *stream) {
    strcpy(buf, "end\n");
    return buf;
}

char *mock_input_send(char *buf, int size, FILE *stream) {
    strcpy(buf, "send message\n");
    return buf;
}

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

void test_execute_client_action_success() {
    ClientContext context;
    client_context_init(&context);
    char buffer[BUFFER_SIZE] = "send message";
    transaction_result res =
        execute_client_action(&context, INPUT_ACTION_SEND, buffer, mock_recv_success, mock_send_success);
    TEST_ASSERT_EQUAL(TRANSACTION_SUCCESS, res);
}

void test_execute_client_action_server_closed() {
    ClientContext context;
    client_context_init(&context);
    char buffer[BUFFER_SIZE] = "send message";
    transaction_result res =
        execute_client_action(&context, INPUT_ACTION_SEND, buffer, mock_recv_closed, mock_send_success);
    TEST_ASSERT_EQUAL(TRANSACTION_SERVER_CLOSED, res);
}

void test_execute_client_action_recv_error() {
    ClientContext context;
    client_context_init(&context);
    char buffer[BUFFER_SIZE] = "send message";
    transaction_result res = execute_client_action(&context, INPUT_ACTION_SEND, buffer, tcp_recv, mock_send_success);
    TEST_ASSERT_EQUAL(TRANSACTION_ERROR, res);
}

void test_start_communication_fails_uninitialized_socket() {
    ClientContext context;
    client_context_init(&context);
    context.tcp_socket = -1; // Simulate an uninitialized socket
    int res = start_communication(&context, tcp_recv, tcp_send, mock_input_end);
    TEST_ASSERT_EQUAL(-1, res); // Expect failure due to uninitialized socket
}

void test_start_communication_user_quit() {
    ClientContext context;
    client_context_init(&context);
    context.tcp_socket = 1;

    int res = start_communication(&context, mock_recv_success, mock_send_success, mock_input_end);
    TEST_ASSERT_EQUAL(0, res);
}

void test_start_communication_send_error() {
    ClientContext context;
    client_context_init(&context);
    context.tcp_socket = 1;

    // Simulate a send error
    int res = start_communication(&context, mock_recv_success, mock_send_error, mock_input_send);
    TEST_ASSERT_EQUAL_INT(-1, res);
}

void test_launch_dashboard_fails_without_clientID() {
    ClientContext context;
    client_context_init(&context);
    client_context_set_id(&context, NULL); // No client ID set
    bool result = launch_dashboard(&context);
    TEST_ASSERT_FALSE(result);
}