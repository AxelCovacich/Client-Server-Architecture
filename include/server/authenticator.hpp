#ifndef AUTHENTICATOR_HPP
#define AUTHENTICATOR_HPP

#include <string>

/**
 * @class Authenticator
 * @brief Handles client authentication logic.
 */
class Authenticator {
  public:
    /**
     * @brief Authenticates a client based on provided credentials.
     * @param hostname The client's hostname, used as a username.
     * @param password The client's password.
     * @return True if authentication is successful, false otherwise.
     */
    bool authenticate(const std::string &hostname, const std::string &password) const;
};

#endif // AUTHENTICATOR_HPP