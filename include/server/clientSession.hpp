#ifndef CLIENT_SESSION_HPP
#define CLIENT_SESSION_HPP

#include "authenticator.hpp"
#include "config.hpp"
#include "eventQueue.hpp"
#include "inventory.hpp"
#include "logger.hpp"
#include "sessionManager.hpp"
#include "trafficReporter.hpp"
#include "udpHandler.hpp"
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <sys/socket.h>

#define WaitForDataSleepMs 100

using json = nlohmann::json;

/**
 * @class ClientSession
 * @brief Manages the entire lifecycle and state of a single client connection.
 *
 * Each instance of this class is responsible for one connected client. It handles
 * the authentication handshake, the main command processing loop, and ensures
 * resources are cleaned up upon disconnection.
 */
class clientSession : public std::enable_shared_from_this<clientSession> {
  private:
    int m_clientSocket;
    std::string m_clientID;
    bool m_isAuthenticated;
    std::string m_clientIP;
    std::shared_ptr<sockaddr_storage> m_udpAddress;
    mutable std::mutex m_sessionMutex;
    std::deque<std::string> m_pendingMessages;

    // need to be by reference for mutex use. Can't be copies of the object
    TrafficReporter &m_trafficReporter;
    Inventory &m_inventory;
    Authenticator &m_authenticator;
    EventQueue &m_eventQueue;
    Logger &m_logger;
    Storage &m_storage;
    const Config &m_config;
    SessionManager &m_sessionManager;
    UdpHandler &m_udpHandler;

  public:
    /**
     * @brief Constructs a new ClientSession.
     * @param client_socket The file descriptor for the connected client's socket.
     * @param inventory A reference to the shared server inventory.
     * @param authenticator A reference to the shared authenticator.
     * @param logger A reference to the shared system logger.
     * @param clientIP The IP from the connected client.
     * @param storage A reference to the shared database storage.
     * @param sessionManager A reference to the shared session manager.
     * @param config A reference to the shared server configuration.
     * @param trafficReporter A reference to the shared traffic reporter.
     * @param eventQueue A reference to the shared event queue.
     * @param udpHandler A reference to the shared UDP handler.
     * @throw std::exception if a critical setup step fails.
     */
    clientSession(int clientSocket, Inventory &inventory, Authenticator &autenthicator, Logger &logger,
                  Storage &storage, const std::string &clientIP, SessionManager &sessionManager, const Config &config,
                  TrafficReporter &trafficReporter, EventQueue &eventQueue, UdpHandler &udpHandler);

    ~clientSession();

    /**
     * @brief The main entry point for the session's logic.
     *
     * This function contains the main communication functions that reads commands from the client,
     * processes them, and send responses. This is intended to be run by worker threads from the thread pool.
     * @return True if the session should continue, false if it should be terminated.
     */
    bool run();

    /**
     * @brief Gets the current authentication status of the session.
     * @return True if the session is authenticated, false otherwise.
     */
    bool isAuthenticated() const;

    // private bc its implemented inside run() method
    // return the pair <string answer, bool continue?>
    using processResult = std::pair<std::string, bool>;

    /**
     * @brief Processes a single raw JSON message from the client.
     *
     * This method acts as the central state machine for the session. It handles
     * the authentication logic for unauthenticated clients and dispatches commands
     * to the CommandProcessor for authenticated clients. It also contains the
     * top-level exception handler for critical server errors.
     *
     * @param json_string The raw message received from the client's socket.
     * @return A ProcessResult pair containing the JSON string response and a boolean
     * indicating if the connection should be kept alive.
     */
    processResult processMessage(const std::string &json_string);

    /**
     * @brief Creates a sanitized version of a request for safe logging.
     *
     * This static utility function takes a JSON request object and redacts any
     * sensitive fields (e.g., "password") before it is logged.
     * @param request The original JSON request object.
     * @return A new JSON object with sensitive data masked.
     */
    static json createLoggableRequest(json request);

    /**
     * @brief Sets the UDP address for this client session.
     * @param addr The sockaddr_storage structure containing the UDP address.
     */
    void setUdpAddress(const struct sockaddr_storage &addr);

    /**
     * @brief Gets the UDP address associated with this client session.
     * @return A shared pointer to the sockaddr_storage containing the UDP address.
     */
    std::shared_ptr<sockaddr_storage> getUdpAddress() const;

    /**
     * @brief Handles and processes events from the event queue for this client.
     * @return True if an event was handled successfully, false otherwise.
     */
    bool handleEventQueue();

    /**
     * @brief Attempts to send any pending messages to the client.
     * @return True if all pending messages were sent successfully, false otherwise.
     */
    bool trySendPendingMessage();

    /**
     * @brief Checks if there are any pending messages to be sent to the client.
     * @return True if there are pending messages, false otherwise.
     */
    bool hasPendingMessages() const;

    /**
     * @brief Sends a welcome message to the client upon connection.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool sendWelcomeMessage();
};

#endif // CLIENT_SESSION_HPP
