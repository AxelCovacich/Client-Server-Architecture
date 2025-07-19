#include "authenticator.hpp"

bool Authenticator::authenticate(const std::string &hostname, const std::string &password) const {

    // hardcoded values for initial implementation
    if (hostname == "ubuntu" && password == "ubuntu") {
        return true;
    }

    if (hostname == "warehouse-A" && password == "pass123") {
        return true;
    }

    return false;
}