#ifndef SERVER_HPP
#define SERVER_HPP

#include "authenticator.hpp"
#include "inventory.hpp"
#include <string>
#include <utility>
#define BUFFER_SIZE 256

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
    Server(int port);

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
    int port_;
    int server_fd_; // File descriptor for the listening socket

    Inventory m_inventory;
    Authenticator m_authenticator;

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