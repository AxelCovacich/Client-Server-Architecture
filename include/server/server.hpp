#ifndef SERVER_HPP
#define SERVER_HPP

#include "alertManager.hpp"
#include "authenticator.hpp"
#include "clientSession.hpp"
#include "config.hpp"
#include "eventQueue.hpp"
#include "inventory.hpp"
#include "ipcHandler.hpp"
#include "logger.hpp"
#include "sessionManager.hpp"
#include "threadPool.hpp"
#include "trafficReporter.hpp"
#include "udpHandler.hpp"
#include <string>
#include <utility>

#define BUFFER_SIZE 256
#define MAX_EPOLL_EVENTS 1024
#define EPOLL_WAIT_TIMEOUT_MS 1000
#define THREAD_POOL_SIZE 16

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
     * @param trafficReporter A reference to the traffic reporting module.
     * @throw std::exception if a critical setup step fails (e.g., socket bind, DB open).
     */
    Server(const Config &config, const IClock &clock, Storage &storage, Logger &logger,
           TrafficReporter &trafficReporter);

    /**
     * @brief Destroys the Server object.
     * * Ensures the listening server socket is properly closed (RAII).
     */
    ~Server();

    /**
     * @brief Starts the main server loop.
     * Enters an infinite loop to accept and handle incoming client connections.
     * Each client is handled in a separate thread from the thread pool.
     * This function blocks indefinitely until the server is shut down.
     * @throw std::exception if a critical runtime error occurs.
     */
    void run();

    /**
     * @brief Retrieves the client's IP address as a string.
     * @param clientAddress The sockaddr_storage structure containing the client's address.
     * @return A string representation of the client's IP address.
     */
    static std::string getClientIP(const struct sockaddr_storage &clientAddress);

  private:
    int m_tcpPort;
    int m_udpPort;
    int m_serverTCPFD;
    int m_serverUDPFD;
    int m_serverUnixFD;
    int m_epollFD;
    std::unordered_map<int, std::shared_ptr<clientSession>> m_clientSessions; // map of client socket fd to its session
    std::unordered_set<int> m_unixConnections;                                // set of active unix socket fds

    const Config &m_config;
    const IClock &m_clock;
    Storage &m_storage;
    Logger &m_logger;
    TrafficReporter &m_trafficReporter;

    ThreadPool m_threadPool;
    EventQueue m_eventQueue;
    Inventory m_inventory;
    Authenticator m_authenticator;
    AlertManager m_alert;
    SessionManager m_sessionManager;
    UdpHandler m_udpHandler;
    IpcHandler m_ipcHandler;
    /**
     * @brief Sets up the server's listening socket.
     * * Performs the socket(), bind(), and listen() sequence.
     * @throw std::runtime_error If any step in the socket setup fails.
     */
    void setupServer();

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
     * @brief Handles incoming TCP connections. Adds new client sockets to epoll and creates sessions.
     */
    void handleTcpConnection();

    /**
     * @brief Sets up the UNIX domain socket for IPC.
     */
    void setUNIXconfig();

    /**
     * @brief Handles incoming IPC connections on the UNIX domain socket.
     * Accepts new connections and delegates handling to the IpcHandler in a thread pool task.
     */
    void handleUNIXConnection();

    /**
     * @brief Handles activity on a client TCP socket.
     * Reads incoming data and processes client messages in the associated client session with the thread pool task.
     * @param clientFileDescriptor The file descriptor of the client socket with activity.
     */
    void tcpHandling(int clientFileDescriptor);

    /**
     * @brief Processes pending messages for a client.
     * @param clientFileDescriptor The file descriptor of the client socket.
     */
    void pendingMessages(int clientFileDescriptor);
};

#endif // SERVER_HPP