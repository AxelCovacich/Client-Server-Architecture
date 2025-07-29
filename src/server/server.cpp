
#include "server.hpp"
#include <arpa/inet.h>
#include <commandProcessor.hpp>
#include <csignal> // For std::signal
#include <cstring> // For memset()
#include <iostream>
#include <memory> //for make_shared
#include <netinet/in.h>
#include <stdexcept> // For std::runtime_error
#include <string>
#include <sys/socket.h>
#include <thread>   // For jthread
#include <unistd.h> // For close()
#include <utility>

using namespace std;

/**
 * @brief Global flag to indicate if a shutdown signal has been received.
 * * This variable is volatile and sig_atomic_t to ensure it is safe to access
 * from a signal handler.
 */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
volatile sig_atomic_t g_shutdown_flag = 0;

/**
 * @brief Signal handler function to flag graceful shutdown.
 * @param signum The signal number received.
 */
void signal_handler(int signum) {
    g_shutdown_flag = 1;
}

Server::Server(int port, Inventory &inventory, Authenticator &authenticator, Logger &logger, Storage &storage)
    : m_port(port)
    , m_storage(storage)
    , m_serverFD(-1)
    , m_logger(logger)
    , m_authenticator(authenticator)
    , m_inventory(inventory) {

    setupServer();
}

Server::~Server() {
    if (m_serverFD != -1) {
        close(m_serverFD);
    }
}

void Server::setupServer() {
    m_serverFD = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFD < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "Error while trying to create socket...");
        throw runtime_error("Failed to create socket");
    }

    struct sockaddr_in serv_addr {};
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(m_port);

    // Necessary to interact with the old C Socket API
    //  NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (bind(m_serverFD, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "Error while trying to bind socket.");
        throw runtime_error("Failed to bind to port");
    }

    if (listen(m_serverFD, MAX_CLIENTS_PERMITTED) < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "Error while trying to listen on socket.");
        throw runtime_error("Failed to listen on socket");
    }
    m_logger.log(LogLevel::INFO, "Server", "Server listenig on port " + std::to_string(m_port));
    // cout << "Server listening on port: " << port_ << '\n';
}

void Server::run() {
    fd_set read_fds;
    struct timeval timevalue {};

    struct sockaddr_in cli_addr {};
    socklen_t clilen = sizeof(cli_addr);

    while (g_shutdown_flag == 0) {
        FD_ZERO(&read_fds);
        FD_SET(m_serverFD, &read_fds);

        timevalue.tv_sec = 1;
        timevalue.tv_usec = 0;

        // use select to wait for 1 sec or a clients connects and then go for another iteration

        int activity = select(m_serverFD + 1, &read_fds, NULL, NULL, &timevalue);

        if ((activity < 0) && (errno != EINTR)) {
            m_logger.log(LogLevel::ERROR, "Server", "Error while trying to select.");
            // perror("select error");
        }

        if (activity > 0 && FD_ISSET(m_serverFD, &read_fds)) {
            socklen_t clilen = sizeof(cli_addr);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            int newsockfd = accept(m_serverFD, reinterpret_cast<struct sockaddr *>(&cli_addr),
                                   &clilen); // Necessary to interact with API sockets of C

            if (newsockfd < 0) {
                m_logger.log(LogLevel::ERROR, "Server", "Error while trying to accept.");
                // perror("Error on accept");
                continue;
            }

            std::array<char, INET_ADDRSTRLEN> clientIPArray{};

            // Convert binary IP to text
            if (inet_ntop(AF_INET, &cli_addr.sin_addr, clientIPArray.data(), clientIPArray.size()) == nullptr) {
                m_logger.log(LogLevel::ERROR, "Server", "Error while trying to convert client IP.");
                throw std::system_error(errno, std::system_category(), "inet_ntop failed converting client IP");
            }
            // this creates a new clientSession object in dinamic memory (new). It wrappes it in a shared_ptr and
            // returns no need to manually delete, local variable session finish and deletes its own ptr. copies of
            // shared_ptr are used by threads independently of local variable session. When all instances(threads)
            // complete their jobs and there is no more copies of the object clientSession via shared_ptr, memory will
            // be freed automaticly.
            auto session = std::make_shared<clientSession>(newsockfd, m_inventory, m_authenticator, m_logger, m_storage,
                                                           std::string(clientIPArray.data()));
            jthread client_thread(&clientSession::run, session);
            client_thread.detach();
        }
    }
    // cout << "\nShutdown signal received. Server is closing.\n";
    m_logger.log(LogLevel::INFO, "Server", "Shutdown signal received. Server is now closing.");
}
