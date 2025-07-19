#ifndef CLIENT_SESSION_HPP
#define CLIENT_SESSION_HPP

#include "authenticator.hpp"
#include "inventory.hpp"
#include <string>

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

    // need to be by reference for mutex use. Can't be copies of the object
    Inventory &m_inventory;
    Authenticator &m_authenticator;

  public:
    /**
     * @brief Constructs a new ClientSession.
     * @param client_socket The file descriptor for the connected client's socket.
     * @param inventory A reference to the shared server inventory.
     * @param authenticator A reference to the shared authenticator.
     */
    clientSession(int clientSocket, Inventory &inventory, Authenticator &autenthicator);

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
    processResult processMessage(const std::string &json_string);
};

#endif // CLIENT_SESSION_HPP
