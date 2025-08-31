#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "logger.hpp"
#include "storage.hpp"
#include "trafficReporter.hpp"
#include <map>
#include <memory> // std::shared_ptr
#include <mutex>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

class clientSession;

/**
 * @class SessionManager
 * @brief Manages all active client sessions, providing a thread-safe interface.
 *
 * This class is a singleton-like service that tracks all connected clients.
 * It is used by the Alerting module to broadcast messages and by the
 * Authenticator/Session modules to handle client-specific state like locks.
 */
class SessionManager {
  public:
    SessionManager(Storage &storage, Logger &logger, TrafficReporter &trafficReporter);
    /**
     * @brief Registers a new client session.
     * @param clientId The unique ID of the client.
     * @param session A shared pointer to the client's session object.
     */
    void registerSession(const std::string &clientId, std::shared_ptr<clientSession> session);

    /**
     * @brief Unregisters a client session upon disconnection.
     * @param clientId The unique ID of the client to remove.
     */
    void unregisterSession(const std::string &clientId);

    /**
     * @brief Locks a client account that have triggered an alert, preventing further actions.
     * @param clientId The ID of the client to lock.
     */
    bool lockClient(const std::string &clientId);

    /**
     * @brief Checks if a client account is currently locked.
     * @param clientId The ID of the client to check.
     * @return True if the client is locked, false otherwise.
     */
    bool isClientLocked(const std::string &clientId);

    /**
     * @brief Retrieves the network addresses of all active clients for broadcasting.
     * @return A vector of strings containing the IP addresses of all connected clients.
     */
    std::vector<struct sockaddr_storage> getActiveUdpAddresses();

    bool unlockClient(const std::string &clientId);

    /**
     * @brief Checks if a client is currently registered in the active session map.
     * @param clientId The ID of the client to check.
     * @return True if the client is registered, false otherwise.
     */
    bool isClientRegistered(const std::string &clientId);

    std::shared_ptr<clientSession> getSession(const std::string &clientId);

  private:
    Storage &m_storage;
    Logger &m_logger;
    TrafficReporter &m_trafficReporter;
    // Mutex to protect access to the shared maps and sets from multiple threads.
    std::mutex m_mutex;

    // Maps a client ID to its active session object.
    std::map<std::string, std::shared_ptr<clientSession>> m_activeSessions;

    // A set to quickly check which client IDs are currently locked.
    std::set<std::string> m_lockedClients;

    std::unordered_set<std::string> m_connectedUsers; // Users that have connected once
};

#endif // SESSION_MANAGER_HPP