#ifndef SERVER_HPP
#define SERVER_HPP

#include "authenticator.hpp"
#include "clientSession.hpp"
#include "inventory.hpp"
#include "logger.hpp"
#include <string>
#include <utility>

#define BUFFER_SIZE 256
#define BUFFER_SIZE_UDP 256
#define MAX_CLIENTS_PERMITTED 1000

/**
 * @class Server
 * @brief Manages a TCP server that can handle multiple clients concurrently.
 */
class Server {
  public:
    /**
     * @brief Constructs a new Server object and its core service modules.
     *
     * Initializes all dependencies (Storage, Logger, etc.) and sets up the
     * network listeners. Throws an exception if critical setup fails.
     * @param port The base TCP port number to listen on.
     * @param dbPath The file path for the SQLite database.
     * @param inventory A reference to the application's inventory management module.
     * @param authenticator A reference to the authentication module.
     * @param logger A reference to the logging module.
     * @throw std::exception if a critical setup step fails (e.g., socket bind, DB open).
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
    int m_serverTCPFD; // File descriptor for the listening socket TCP
    int m_serverUDPFD;

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

    /**
     * @brief Sets up the dual-stack (IPv4/IPv6) TCP listening socket.
     *
     * This function creates, configures (with SO_REUSEADDR and IPV6_V6ONLY=off),
     * binds, and listens on the main TCP socket.
     */
    void setTCPConfig();

    /**
     * @brief Sets up the dual-stack (IPv4/IPv6) UDP listening socket.
     *
     * This function creates and binds the secondary UDP socket for connectionless
     * communication (e.g., keepalives).
     */
    void setUDPConfig();

    /**
     * @brief Handles an incoming UDP datagram.
     *
     * This method is called by the main run loop when 'select()' detects activity
     * on the UDP socket. It receives the packet and logs its content.
     */
    void handleUdpMessage();
};

#endif // SERVER_HPP