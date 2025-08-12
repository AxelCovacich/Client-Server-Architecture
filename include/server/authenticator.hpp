#ifndef AUTHENTICATOR_HPP
#define AUTHENTICATOR_HPP

#include "clock.hpp"
#include "logger.hpp"
#include "storage.hpp"
#include <mutex>
#include <string>

enum class AuthResult {
    SUCCESS,
    FAILED_USER_NOT_FOUND,
    FAILED_ACCOUNT_LOCKED,
    FAILED_BAD_CREDENTIALS,
    FAILED_ALERT_LOCKED
};

/**
 * @class Authenticator
 * @brief Handles client authentication logic.
 */
class Authenticator {
  public:
    Authenticator(Storage &storage, const IClock &clock, Logger &logger);
    /* @brief Authenticates a client based on provided credentials.
     * @param hostname The client's hostname, used as a username.
     * @param password The client's password.
     * @return True if authentication is successful, false otherwise.
     */
    AuthResult authenticate(const std::string &hostname, const std::string &password) const;

  private:
    const IClock &m_clock;
    mutable std::mutex m_mutex;
    Storage &m_storage;
    Logger &m_logger;
};

#endif // AUTHENTICATOR_HPP