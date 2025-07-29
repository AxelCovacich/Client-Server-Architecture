#ifndef SERVER_HPP
#define SERVER_HPP

#include "authenticator.hpp"
#include "clientSession.hpp"
#include "inventory.hpp"
#include "logger.hpp"
#include <string>
#include <utility>

#define BUFFER_SIZE 256
#define MAX_CLIENTS_PERMITTED 1000

/**
 * @class Server
 * @brief Manages a TCP server that can handle multiple clients concurrently.
 */
class Server {
  public:
    /**
     * @brief Constructs a new Server object.
     * * Initializes the server to listen on the specified port. Throws an
     * exception if the setup fails (e.g., port is already in use).
     * * @param port The port number to listen on.
     * @throw std::runtime_error If socket creation, binding, or listening fails.
     */
    Server(int port, Inventory &inventory, Authenticator &authenticator, Logger &logger, Storage &storage);

    /**
     * @brief Destroys the Server object.
     * * Ensures the listening server socket is properly closed (RAII).
     */
    ~Server();

    /**
     * @brief Starts the main server loop.
     * * Enters an infinite loop to accept and handle incoming client connections.
     * Each client is handled in a separate thread.
     */
    void run();

  private:
    int m_port;
    int m_serverFD; // File descriptor for the listening socket

    Logger &m_logger;
    Authenticator &m_authenticator;
    Inventory &m_inventory;
    Storage &m_storage;

    /**
     * @brief Sets up the server's listening socket.
     * * Performs the socket(), bind(), and listen() sequence.
     * @throw std::runtime_error If any step in the socket setup fails.
     */
    void setupServer();

    /**
     * @brief Handles communication with a single client.
     * * This function is executed in a separate thread for each client.
     * * @param client_socket The file descriptor for the connected client's
     * socket.
     */
    void handleClient(int client_socket);
};

#endif // SERVER_HPP