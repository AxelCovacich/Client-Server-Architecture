#include "authenticator.hpp"
#include "bcrypt.h"
#include "storage.hpp"
#include <ctime>
#include <iostream>

using namespace std;

Authenticator::Authenticator(Storage &storage, const IClock &clock, Logger &logger)
    : m_storage(storage)
    , m_clock(clock)
    , m_logger(logger) {
}

AuthResult Authenticator::authenticate(const std::string &hostname, const std::string &password,
                                       int blockDuration) const {

    std::lock_guard<std::mutex> lock(m_mutex); // mutex here to make the secuence of select--> update atomic

    std::optional<userAuthData> authData = m_storage.getUserLoginData(hostname);
    if (!authData.has_value()) {
        // cerr << "No user registered for the given hostname: " << hostname << '\n';
        m_logger.log(LogLevel::WARNING, "Authenticator", "No user found for the given hostname: '" + hostname + "'.");
        return AuthResult::FAILED_USER_NOT_FOUND;
    }

    if (authData->is_locked) {
        return AuthResult::FAILED_ALERT_LOCKED;
    }

    std::time_t currentTime = m_clock.now();

    if (authData->failedAttempts >= 3 && (currentTime - authData->lastFailedTimestamp) < blockDuration) {

        // cerr << "Login temporally blocked for " << hostname << ". Try again later.\n";
        m_logger.log(LogLevel::WARNING, "Authenticator",
                     "User by hostname: '" + hostname + "' is trying to login but is blocked.");
        return AuthResult::FAILED_ACCOUNT_LOCKED;
    }

    if (authData->failedAttempts >= 3) {
        m_storage.updateLoginAttempts(hostname, true,
                                      currentTime); // Blocked window time passed, reset the attempts counter to 0
    }
    if (bcrypt::validatePassword(password, authData->passwordHash)) {
        // successfull login,reset the login attempts
        m_logger.log(LogLevel::INFO, "Authenticator", "Login successful for user '" + hostname + "'.");
        m_storage.updateLoginAttempts(hostname, true, currentTime);

        return AuthResult::SUCCESS;
    }

    m_storage.updateLoginAttempts(hostname, false, currentTime); // Failed, +1 attempt, save time
    if (authData->failedAttempts == 2) {
        m_logger.log(LogLevel::WARNING, "Authenticator",
                     "Failed login attempts limit reached for user '" + hostname + "'. Login is disabled for 15 min.");
        return AuthResult::FAILED_ACCOUNT_LOCKED;
    }
    m_logger.log(LogLevel::WARNING, "Authenticator", "Failed login attempt for user '" + hostname + "'.");
    return AuthResult::FAILED_BAD_CREDENTIALS;
}