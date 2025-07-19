#include "authenticator.hpp"
#include "unity.h"

void testAuthenticatorSucceedsWithValidCredentials() {
    Authenticator auth;
    bool result = auth.authenticate("warehouse-A", "pass123");
    TEST_ASSERT_TRUE(result);
}

void testAuthenticatorFailsWithInvalidPassword() {
    Authenticator auth;
    bool result = auth.authenticate("warehouse-A", "wrong_password");
    TEST_ASSERT_FALSE(result);
}

void testAuthenticatorFailsWithUnknownUser() {
    Authenticator auth;
    bool result = auth.authenticate("unknown-client", "pass123");
    TEST_ASSERT_FALSE(result);
}