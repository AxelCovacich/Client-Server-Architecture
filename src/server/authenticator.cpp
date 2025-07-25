#include "authenticator.hpp"
#include "bcrypt.h"
#include "storage.hpp"
#include <ctime>
#include <iostream>

using namespace std;

Authenticator::Authenticator(Storage &storage, const IClock &clock)
    : m_storage(storage)
    , m_clock(clock) {
}

bool Authenticator::authenticate(const std::string &hostname, const std::string &password) const {

    std::lock_guard<std::mutex> lock(m_mutex); // mutex here to make the secuence of select--> update atomic

    std::optional<userAuthData> authData = m_storage.getUserLoginData(hostname);
    if (!authData.has_value()) {
        cerr << "No user registered for the given hostname: " << hostname << '\n';
        return false;
    }

    const long BLOCK_DURATION_SECONDS = 15 * 60; // 15 minutos
    std::time_t currentTime = m_clock.now();

    if (authData->failedAttempts >= 3 && (currentTime - authData->lastFailedTimestamp) < BLOCK_DURATION_SECONDS) {

        cerr << "Login temporally blocked for " << hostname << ". Try again later.\n";
        return false;
    }

    if (authData->failedAttempts >= 3) {
        m_storage.updateLoginAttempts(hostname, true,
                                      currentTime); // Blocked window time passed, reset the attempts counter to 0
    }
    if (bcrypt::validatePassword(password, authData->passwordHash)) {
        // successfull login,reset the login attempts
        m_storage.updateLoginAttempts(hostname, true, currentTime);
        return true;
    }

    m_storage.updateLoginAttempts(hostname, false, currentTime); // Failed, +1 attempt, save time
    return false;
}