#ifndef AUTHENTICATOR_HPP
#define AUTHENTICATOR_HPP

#include "clock.hpp"
#include "storage.hpp"
#include <mutex>
#include <string>

/**
 * @class Authenticator
 * @brief Handles client authentication logic.
 */
class Authenticator {
  public:
    Authenticator(Storage &storage, const IClock &clock);
    /**
     * @brief Authenticates a client based on provided credentials.
     * @param hostname The client's hostname, used as a username.
     * @param password The client's password.
     * @return True if authentication is successful, false otherwise.
     */
    bool authenticate(const std::string &hostname, const std::string &password) const;

  private:
    const IClock &m_clock;
    mutable std::mutex m_mutex;
    Storage &m_storage;
};

#endif // AUTHENTICATOR_HPP