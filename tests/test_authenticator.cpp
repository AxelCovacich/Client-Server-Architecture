#include "authenticator.hpp"
#include "clock.hpp"
#include "storage.hpp"
#include "unity.h"

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
    Authenticator authenticator(storage, clock);

    storage.createUser("warehouse-A", "pass123");

    TEST_ASSERT_TRUE(authenticator.authenticate("warehouse-A", "pass123"));
}

void testAuthenticatorFailsWithInvalidPassword() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Authenticator authenticator(storage, clock);

    storage.createUser("warehouse-A", "pass123");

    TEST_ASSERT_FALSE(authenticator.authenticate("warehouse-A", "invalid_password"));
}

void testAuthenticatorFailsWithUnknownUser() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Authenticator authenticator(storage, clock);

    storage.createUser("warehouse-A", "pass123");

    TEST_ASSERT_FALSE(authenticator.authenticate("somehostname", "pass123"));
}

void testAuthenticatorFailsWithFailedAttempts() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Authenticator authenticator(storage, clock);

    storage.createUser("warehouse-A", "pass123");
    authenticator.authenticate("warehouse-A", "pass122");
    authenticator.authenticate("warehouse-A", "pass125");
    authenticator.authenticate("warehouse-A", "pass124");

    TEST_ASSERT_FALSE(authenticator.authenticate("warehouse-A", "pass123"));
}
void testAuthenticatorResetsFailedAttempts() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Authenticator authenticator(storage, clock);
    std::string hostname = "warehouse-A";

    storage.createUser(hostname, "pass123");
    authenticator.authenticate(hostname, "pass122");
    authenticator.authenticate(hostname, "pass125");

    TEST_ASSERT_TRUE(authenticator.authenticate(hostname, "pass123"));

    std::optional<userAuthData> userData = storage.getUserLoginData(hostname);
    TEST_ASSERT_TRUE(userData.has_value());
    TEST_ASSERT_EQUAL_INT(0, userData->failedAttempts);
}

void testAuthenticatorPassAfterBlockedTime() {
    Storage storage(":memory:");
    storage.initializeSchema();
    MockClock clock;
    Authenticator authenticator(storage, clock);
    std::string hostname = "warehouse-A";

    storage.createUser(hostname, "pass123");
    authenticator.authenticate(hostname, "pass122");
    authenticator.authenticate(hostname, "pass125");
    clock.set_time(1000000000);
    storage.updateLoginAttempts(hostname, false, clock.now());

    const long BLOCK_DURATION_SECONDS = 15 * 60;
    clock.set_time(1000000000 + BLOCK_DURATION_SECONDS + 1); // 15 min pass simulation

    TEST_ASSERT_TRUE(authenticator.authenticate(hostname, "pass123"));

    std::optional<userAuthData> userData = storage.getUserLoginData(hostname);
    TEST_ASSERT_TRUE(userData.has_value());
    TEST_ASSERT_EQUAL_INT(0, userData->failedAttempts);
}