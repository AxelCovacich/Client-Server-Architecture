#include "sessionManager.hpp"
#include "clientSession.hpp"

SessionManager::SessionManager(Storage &storage, Logger &logger, TrafficReporter &trafficReporter)
    : m_storage(storage)
    , m_logger(logger)
    , m_trafficReporter(trafficReporter) {
}

void SessionManager::registerSession(const std::string &clientId, std::shared_ptr<clientSession> session) {

    std::lock_guard<std::mutex> lock(m_mutex);
    // std::move to "transfere" the property of session shared_ptr to the entry of the map
    // without doing a copy. Optimization change suggested by sonnarqube
    m_activeSessions[clientId] = std::move(session);
    m_offlineUsers.erase(clientId);

    if (m_connectedUsers.find(clientId) == m_connectedUsers.end()) {
        m_connectedUsers.insert(clientId); // mark user as connected at least once
    } else {
        // User has reconnected
        m_trafficReporter.incrementReconnection("tcp", "rx");
        m_logger.log(LogLevel::INFO, "SessionManager", "User '" + clientId + "' has reconnected.");
    }
}

void SessionManager::unregisterSession(const std::string &clientId) {

    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeSessions.erase(clientId);
    m_offlineUsers.insert(clientId);
}

bool SessionManager::lockClient(const std::string &clientId) {

    bool success = m_storage.setClientLockStatus(clientId, true);

    if (success) {

        m_logger.log(LogLevel::INFO, "SessionManager",
                     "Client '" + clientId + "' has been locked due to alert trigger.", clientId);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lockedClients.insert(clientId);
        return success;
    }
    m_logger.log(LogLevel::INFO, "SessionManager",
                 "Couldn't lock client '" + clientId + "'. Client was not found. May not be registered.", clientId);
    return success;
}

bool SessionManager::unlockClient(const std::string &clientId) {

    bool success = m_storage.setClientLockStatus(clientId, false);

    if (success) {
        m_logger.log(LogLevel::INFO, "SessionManager", "Client '" + clientId + "' has been unlocked.", clientId);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lockedClients.erase(clientId);
        return success;
    }
    m_logger.log(LogLevel::INFO, "SessionManager",
                 "Couldn't unlock client '" + clientId + "'. Client was not found. May not be registered.", clientId);
    return success;
}

bool SessionManager::isClientLocked(const std::string &clientId) {

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_lockedClients.contains(clientId)) {
        // Cache Hit!
        return true;
    }

    // Cache Miss
    if (m_storage.isClientLocked(clientId)) {

        m_lockedClients.insert(clientId); // cache update
        return true;
    }

    return false;
}

bool SessionManager::isClientRegistered(const std::string &clientId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_activeSessions.contains(clientId);
}

std::shared_ptr<clientSession> SessionManager::getSession(const std::string &clientId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto result = m_activeSessions.find(clientId);

    if (result != m_activeSessions.end()) {

        return result->second;
    }
    return nullptr;
}

std::vector<struct sockaddr_storage> SessionManager::getActiveUdpAddresses() {
    // need to use vector because we may have multiple UDP addresses per session. Use of set would require hash
    // definition also.
    std::vector<struct sockaddr_storage> addresses;
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto &pair : m_activeSessions) {
        auto udp_addr_ptr = pair.second->getUdpAddress();
        if (udp_addr_ptr) {
            addresses.push_back(*udp_addr_ptr);
        }
    }
    return addresses;
}

std::unordered_set<std::string> SessionManager::getOfflineUsers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_offlineUsers;
}