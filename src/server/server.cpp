
#include "server.hpp"
#include <arpa/inet.h>
#include <commandProcessor.hpp>
#include <csignal> // For std::signal
#include <cstring> // For memset()
#include <iostream>
#include <memory>  //for make_shared
#include <netdb.h> //getaddrinfo
#include <netinet/in.h>
#include <stdexcept> // For std::runtime_error
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
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

Server::Server(const Config &config, const IClock &clock, Storage &storage, Logger &logger,
               TrafficReporter &trafficReporter)
    : m_config(config)
    , m_tcpPort(config.getTcpPort())
    , m_udpPort(config.getUdpPort())
    , m_clock(clock)
    , m_storage(storage)
    , m_logger(logger)
    , m_inventory(m_storage, m_logger)
    , m_authenticator(m_storage, m_clock, m_logger)
    , m_trafficReporter(trafficReporter)
    , m_sessionManager(m_storage, m_logger, m_trafficReporter)
    , m_serverTCPFD(-1)
    , m_serverUDPFD(-1)
    , m_serverUnixFD(-1)
    , m_eventQueue(config.getQueueSize())
    , m_udpHandler(m_serverUDPFD, m_logger, m_sessionManager, m_trafficReporter, m_eventQueue)
    , m_alert(m_logger, m_sessionManager, m_udpHandler)
    , m_ipcHandler(m_logger, m_alert, m_trafficReporter) {

    setupServer();
    m_trafficReporter.startPrometheusExposer(config.getMetricHostPort());
}

Server::~Server() {
    if (m_serverTCPFD != -1) {
        std::cout << "Closing TCP server socket...\n";
        close(m_serverTCPFD);
    }
    if (m_serverUDPFD != -1) {
        std::cout << "Closing UDP server socket...\n";
        close(m_serverUDPFD);
    }
    if (m_serverUnixFD != -1) {
        std::cout << "Closing IPC server socket...\n";
        close(m_serverUnixFD);
        unlink("/tmp/server_ipc.sock");
    }
    m_logger.closeLogFile();
}

void Server::setupServer() {

    setTCPConfig();
    setUDPConfig();
    setUNIXconfig();
}

// TODO: migrate to epoll + worker pool
void Server::run() {
    fd_set read_fds;
    struct timeval timevalue {};

    while (g_shutdown_flag == 0) {
        FD_ZERO(&read_fds);
        FD_SET(m_serverTCPFD, &read_fds);
        FD_SET(m_serverUDPFD, &read_fds);
        FD_SET(m_serverUnixFD, &read_fds);

        timevalue.tv_sec = 1;
        timevalue.tv_usec = 0;
        // use select to wait for 1 sec or a clients connects and then go for another iteration

        // First param of select must be the higher+1 descriptor
        int partialMaxFD = std::max(m_serverTCPFD, m_serverUDPFD); // higher of the two will compare against unix
        int maxFD = std::max(partialMaxFD, m_serverUnixFD) + 1;

        int activity = select(maxFD, &read_fds, NULL, NULL, &timevalue);

        if ((activity < 0) && (errno != EINTR)) {
            // m_logger.log(LogLevel::ERROR, "Server", "Error while trying to select.");
            perror("select error");
        }

        if (activity > 0 && FD_ISSET(m_serverTCPFD, &read_fds)) {
            handleTcpConnection();
        }

        if (activity > 0 && FD_ISSET(m_serverUDPFD, &read_fds)) {
            // cout << "UDP activity detected, handling messages...\n";
            m_udpHandler.handleMessage();
        }

        if (activity > 0 && FD_ISSET(m_serverUnixFD, &read_fds)) {
            handleUNIXConnection();
        }
    }
    cout << "\nShutdown signal received. Server is now closing.\n";
    m_logger.log(LogLevel::INFO, "Server", "Shutdown signal received. Server is now closing.");
}

void Server::setTCPConfig() {

    struct addrinfo hints {};
    struct addrinfo *resultList = nullptr;
    struct addrinfo *iterator = nullptr;
    int IPv6_onlyOff = 0;
    int setOption = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6; // We choose IPv6 for gettadrinfo to give back IPv6 results, then turn off the IPv6_only
                                // option to accept IPv4 also
    hints.ai_socktype = SOCK_STREAM; // TCP Socket
    hints.ai_flags = AI_PASSIVE;     // Use my IP to listen

    // Obtain a list of possible directions
    // Use 'nullptr' for host, to listen on all interfaces.
    // Use stored port number in m_port.
    if (getaddrinfo(nullptr, std::to_string(m_tcpPort).c_str(), &hints, &resultList) != 0) {
        m_logger.log(LogLevel::ERROR, "Server", "getaddrinfo failed.");
        throw std::runtime_error("getaddrinfo failed");
    }

    // Iterate over the result list and select the first that works(WARNING first default is ipv4 only, must choose a
    // ipv6 with ipv6_only off)
    for (iterator = resultList; iterator != nullptr; iterator = iterator->ai_next) {
        // Create the socket with the config getaddrinfo give back in resultList
        m_serverTCPFD = socket(iterator->ai_family, iterator->ai_socktype, iterator->ai_protocol);
        if (m_serverTCPFD < 0) {
            continue; // If it fails, try the next one
        }

        // turn off the ipv6 only option, so socket “::” (wildcard IPv6) will recieve IPv4 “packed”
        // (::ffff:127.0.0.1) and can listen on both v4 and v6 with one FD.
        if (setsockopt(m_serverTCPFD, IPPROTO_IPV6, IPV6_V6ONLY, &IPv6_onlyOff, sizeof(IPv6_onlyOff)) < 0) {
            throw std::system_error(errno, std::system_category(), "setsockopt IPV6_V6ONLY failed");
        }

        // With this socket option we can restart the server without having to wait for the OS to free the port to
        // re-use. This fixes the "Address already in use" problem so its possible to restart the server quickly without
        // failing.
        if (setsockopt(m_serverTCPFD, SOL_SOCKET, SO_REUSEADDR, &setOption, sizeof(setOption)) < 0) {
            throw std::system_error(errno, std::system_category(), "setsockopt to reuse port failed");
        }

        // Try to bind
        if (bind(m_serverTCPFD, iterator->ai_addr, iterator->ai_addrlen) == 0) {
            break; // If the bind succeds,keep it and break the loop.
        }

        close(m_serverTCPFD); // If the bind fails, close the FD and try another result from the list.
    }

    // once the loop is done, free the getadrrinfo result
    freeaddrinfo(resultList);

    // Check if could bind on any of the given results, if not the config failed
    if (iterator == nullptr) {
        m_logger.log(LogLevel::ERROR, "Server", "Failed to bind to any address.");
        throw std::runtime_error("Failed to bind");
    }

    // everything is ready to listen
    if (listen(m_serverTCPFD, m_config.getMaxClients()) < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "Failed to listen on socket.");
        throw std::runtime_error("Failed to listen");
    }

    m_logger.log(LogLevel::INFO, "Server", "Server TCP socket listening on port " + std::to_string(m_tcpPort));
    cout << "Server TCP socket listening on port: " << std::to_string(m_tcpPort) << " with FD: " << m_serverTCPFD
         << '\n';
}

void Server::setUDPConfig() {

    struct addrinfo udp_hints {};
    struct addrinfo *udp_result = nullptr;
    udp_hints.ai_family = AF_INET6;     // dual-stack
    udp_hints.ai_socktype = SOCK_DGRAM; // UDP
    udp_hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(nullptr, std::to_string(m_udpPort).c_str(), &udp_hints, &udp_result) != 0) {
        throw std::runtime_error("getaddrinfo for UDP failed");
    }

    // No need for loop here, first result will work
    m_serverUDPFD = socket(udp_result->ai_family, udp_result->ai_socktype, udp_result->ai_protocol);

    if (m_serverUDPFD < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "Failed to create socket.");
        throw std::runtime_error("Failed to create socket");
    }

    if (bind(m_serverUDPFD, udp_result->ai_addr, udp_result->ai_addrlen) < 0) {

        m_logger.log(LogLevel::ERROR, "Server", "Failed to bind to any address.");
        throw std::runtime_error("Failed to bind");
    }

    freeaddrinfo(udp_result);
    m_udpHandler.setSocketFd(m_serverUDPFD); // Set the socket FD in the UDP handler

    m_logger.log(LogLevel::INFO, "Server", "UDP socket listening on port " + std::to_string(m_udpPort));
    cout << "Server UDP socket listening on port: " << std::to_string(m_udpPort) << " with FD: " << m_serverUDPFD
         << '\n';
}

void Server::handleTcpConnection() {

    // use sockaddr_storage, not sockaddr_in(16 bytes) for both ipv4 and ipv6 clients size addresses.
    struct sockaddr_storage clientAddress {};
    socklen_t clientLength = sizeof(clientAddress);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    int newsockfd = accept(m_serverTCPFD, reinterpret_cast<struct sockaddr *>(&clientAddress),
                           &clientLength); // Necessary to interact with API sockets of C

    if (newsockfd < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "Error while trying to accept.");
        perror("Error on accept");
        return;
    }

    auto clientIp = getClientIP(clientAddress);
    if (clientIp == "UNKNOWN") {

        m_logger.log(LogLevel::ERROR, "Server", "Error while trying to get client IP.");
    }

    // this creates a new clientSession object in dinamic memory (new). It wrappes it in a shared_ptr and
    // returns no need to manually delete, local variable session finish and deletes its own ptr. copies of
    // shared_ptr are used by threads independently of local variable session. When all instances(threads)
    // complete their jobs and there is no more copies of the object clientSession via shared_ptr, memory will
    // be freed automaticly.
    auto session = std::make_shared<clientSession>(newsockfd, m_inventory, m_authenticator, m_logger, m_storage,
                                                   clientIp, m_sessionManager, m_config, m_trafficReporter);
    jthread client_thread(&clientSession::run, session);
    client_thread.detach();
}

void Server::setUNIXconfig() {

    const char *socketPath = "/tmp/server_ipc.sock";

    m_serverUnixFD = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_serverUnixFD < 0) {
        throw std::runtime_error("Failed to create UNIX socket");
    }

    struct sockaddr_un server_addr {};
    server_addr.sun_family = AF_UNIX;

    // necssary to interact with C API
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    strncpy(server_addr.sun_path, socketPath, sizeof(server_addr.sun_path) - 1);

    // erase socket file if already exists from a previus run.
    // This prevents the "Adress already in use" error for unix sockets
    unlink(socketPath);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (bind(m_serverUnixFD, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind IPC socket");
    }

    if (listen(m_serverUnixFD, m_config.getMaxUnixConnections()) < 0) {
        throw std::runtime_error("Failed to listen on IPC socket");
    }

    m_logger.log(LogLevel::INFO, "Server", "IPC socket listening on " + std::string(socketPath));
    cout << "Server IPC socket listening on path: " << socketPath << " with FD: " << m_serverUnixFD << '\n';
}

void Server::handleUNIXConnection() {
    // use sockaddr_storage, not sockaddr_in(16 bytes) for both ipv4 and ipv6 clients size addresses.
    struct sockaddr_un clientAddress {};
    socklen_t clientLength = sizeof(clientAddress);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    int newsockfd = accept(m_serverUnixFD, reinterpret_cast<struct sockaddr *>(&clientAddress),
                           &clientLength); // Necessary to interact with API sockets of C

    if (newsockfd < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "Error while trying to accept.");
        perror("Error on accept");
        return;
    }
    m_logger.log(LogLevel::INFO, "Server", "IPC connection established.");

    if (!m_ipcHandler.handleConnection(newsockfd)) {
        close(newsockfd); // error reading from socket
    }
}

std::string Server::getClientIP(const struct sockaddr_storage &clientAddress) {
    // must be big enough to fit ipv4 and ipv6
    std::array<char, INET6_ADDRSTRLEN> clientIPArray{};

    if (clientAddress.ss_family == AF_INET) {
        const auto *castedClientAddrIPv4 = reinterpret_cast<const struct sockaddr_in *>(&clientAddress); // NOLINT
        if (inet_ntop(AF_INET, &castedClientAddrIPv4->sin_addr, clientIPArray.data(), sizeof(clientIPArray)) ==
            nullptr) {
            return "UNKNOWN"; // Return a default value
        }
    } else if (clientAddress.ss_family == AF_INET6) {

        const auto *castedClientAddrIPv6 = reinterpret_cast<const struct sockaddr_in6 *>(&clientAddress); // NOLINT
        if (inet_ntop(AF_INET6, &castedClientAddrIPv6->sin6_addr, clientIPArray.data(), sizeof(clientIPArray)) ==
            nullptr) {
            return "UNKNOWN";
        }

    } else {
        return "UNKNOWN"; // Unsupported address family
    }
    return std::string(clientIPArray.data());
}
