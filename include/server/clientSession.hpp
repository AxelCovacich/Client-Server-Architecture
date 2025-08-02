#ifndef CLIENT_SESSION_HPP
#define CLIENT_SESSION_HPP

#include "authenticator.hpp"
#include "inventory.hpp"
#include "logger.hpp"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

/**
 * @class ClientSession
 * @brief Manages the entire lifecycle and state of a single client connection.
 *
 * Each instance of this class is responsible for one connected client. It handles
 * the authentication handshake, the main command processing loop, and ensures
 * resources are cleaned up upon disconnection.
 */
class clientSession {
  private:
    int m_clientSocket;
    std::string m_clientID;
    bool m_isAuthenticated;
    std::string m_clientIP;

    // need to be by reference for mutex use. Can't be copies of the object
    Inventory &m_inventory;
    Authenticator &m_authenticator;
    Logger &m_logger;
    Storage &m_storage;

  public:
    /**
     * @brief Constructs a new ClientSession.
     * @param client_socket The file descriptor for the connected client's socket.
     * @param inventory A reference to the shared server inventory.
     * @param authenticator A reference to the shared authenticator.
     * @param logger A reference to the shared system logger.
     * @param clientIP The IP from the connected client.
     */
    clientSession(int clientSocket, Inventory &inventory, Authenticator &autenthicator, Logger &logger,
                  Storage &storage, const std::string &clientIP);

    ~clientSession();

    /**
     * @brief The main entry point for the session's logic.
     *
     * This function contains the main loop that reads commands from the client,
     * processes them, and sends responses. This is intended to be run in a
     * separate thread.
     */
    void run();

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
};

#endif // CLIENT_SESSION_HPP
