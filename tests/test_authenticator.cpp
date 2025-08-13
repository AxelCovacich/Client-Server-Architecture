#include "authenticator.hpp"
#include "clock.hpp"
#include "logger.hpp"
#include "storage.hpp"
#include "unity.h"
#include <iostream>

/**
 * @brief Mock clock for testing. Allows setting a specific time.
 */
class MockClock : public IClock {
  public:
    std::time_t now() const override {
        return m_time;
    }
    void set_time(std::time_t new_time) {
        m_time = new_time;
    }

  private:
    std::time_t m_time = 0;
};

void testAuthenticatorSucceedsWithValidCredentials() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Authenticator authenticator(storage, clock, logger);

    storage.createUser("warehouse-A", "pass123");

    AuthResult result = authenticator.authenticate("warehouse-A", "pass123");
    TEST_ASSERT_EQUAL_INT(AuthResult::SUCCESS, result);
}

void testAuthenticatorFailsWithInvalidPassword() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Authenticator authenticator(storage, clock, logger);

    storage.createUser("warehouse-A", "pass123");

    AuthResult result = authenticator.authenticate("warehouse-A", "pass126");
    TEST_ASSERT_EQUAL_INT(AuthResult::FAILED_BAD_CREDENTIALS, result);
}

void testAuthenticatorFailsWithUnknownUser() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Authenticator authenticator(storage, clock, logger);

    storage.createUser("warehouse-A", "pass123");

    AuthResult result = authenticator.authenticate("some_user", "pass123");
    TEST_ASSERT_EQUAL_INT(AuthResult::FAILED_USER_NOT_FOUND, result);
}

void testAuthenticatorFailsWithFailedAttempts() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Authenticator authenticator(storage, clock, logger);

    storage.createUser("warehouse-A", "pass123");
    authenticator.authenticate("warehouse-A", "pass122");
    authenticator.authenticate("warehouse-A", "pass125");
    authenticator.authenticate("warehouse-A", "pass124");

    AuthResult result = authenticator.authenticate("warehouse-A", "pass123");
    TEST_ASSERT_EQUAL_INT(AuthResult::FAILED_ACCOUNT_LOCKED, result);
}
void testAuthenticatorResetsFailedAttempts() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Authenticator authenticator(storage, clock, logger);
    std::string hostname = "warehouse-A";

    storage.createUser(hostname, "pass123");
    authenticator.authenticate(hostname, "pass122");
    authenticator.authenticate(hostname, "pass125");

    AuthResult result = authenticator.authenticate("warehouse-A", "pass123");
    TEST_ASSERT_EQUAL_INT(AuthResult::SUCCESS, result);

    std::optional<userAuthData> userData = storage.getUserLoginData(hostname);
    TEST_ASSERT_TRUE(userData.has_value());
    TEST_ASSERT_EQUAL_INT(0, userData->failedAttempts);
}

void testAuthenticatorPassAfterBlockedTime() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Logger logger(storage, clock, std::cerr);
    Authenticator authenticator(storage, clock, logger);
    std::string hostname = "warehouse-A";

    storage.createUser(hostname, "pass123");
    authenticator.authenticate(hostname, "pass122");
    authenticator.authenticate(hostname, "pass125");
    clock.set_time(1000000000);
    storage.updateLoginAttempts(hostname, false, clock.now());

    clock.set_time(1000000000 + BLOCK_DURATION_SECONDS + 1); // 15 min pass simulation

    AuthResult result = authenticator.authenticate("warehouse-A", "pass123");
    TEST_ASSERT_EQUAL_INT(AuthResult::SUCCESS, result);

    std::optional<userAuthData> userData = storage.getUserLoginData(hostname);
    TEST_ASSERT_TRUE(userData.has_value());
    TEST_ASSERT_EQUAL_INT(0, userData->failedAttempts);
}