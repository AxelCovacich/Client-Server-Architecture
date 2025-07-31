
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
    , m_serverTCPFD(-1)
    , m_serverUDPFD(-1)
    , m_logger(logger)
    , m_authenticator(authenticator)
    , m_inventory(inventory) {

    setupServer();
}

Server::~Server() {
    if (m_serverTCPFD != -1) {
        close(m_serverTCPFD);
    }
}

void Server::setupServer() {

    setTCPConfig();
    setUDPConfig();
}

// TODO: migrate to epoll + worker pool
void Server::run() {
    fd_set read_fds;
    struct timeval timevalue {};

    // use sockaddr_storage, not sockaddr_in(16 bytes) for both ipv4 and ipv6 clients size addresses.
    struct sockaddr_storage cli_addr {};
    // struct sockaddr_in cli_addr {};
    socklen_t clilen = sizeof(cli_addr);

    while (g_shutdown_flag == 0) {
        FD_ZERO(&read_fds);
        FD_SET(m_serverTCPFD, &read_fds);
        FD_SET(m_serverUDPFD, &read_fds);

        timevalue.tv_sec = 1;
        timevalue.tv_usec = 0;

        // use select to wait for 1 sec or a clients connects and then go for another iteration

        // First param of select must be the higher+1 descriptor
        int maxFD = std::max(m_serverTCPFD, m_serverUDPFD) + 1;

        int activity = select(maxFD, &read_fds, NULL, NULL, &timevalue);

        if ((activity < 0) && (errno != EINTR)) {
            // m_logger.log(LogLevel::ERROR, "Server", "Error while trying to select.");
            perror("select error");
        }

        if (activity > 0 && FD_ISSET(m_serverTCPFD, &read_fds)) {

            socklen_t clilen = sizeof(cli_addr);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            int newsockfd = accept(m_serverTCPFD, reinterpret_cast<struct sockaddr *>(&cli_addr),
                                   &clilen); // Necessary to interact with API sockets of C

            if (newsockfd < 0) {
                // m_logger.log(LogLevel::ERROR, "Server", "Error while trying to accept.");
                perror("Error on accept");
                continue;
            }

            // must be big enough to fit ipv4 and ipv6
            std::array<char, INET6_ADDRSTRLEN> clientIPArray{};

            // Convert binary IP to text depending on if its ipv4 or ipv6
            if (cli_addr.ss_family == AF_INET) {
                auto *castedClientAddrIPv4 = reinterpret_cast<struct sockaddr_in *>(&cli_addr);

                if (inet_ntop(AF_INET, &castedClientAddrIPv4->sin_addr, clientIPArray.data(), sizeof(clientIPArray)) ==
                    nullptr) {
                    m_logger.log(LogLevel::ERROR, "Server", "Error while trying to convert client IPv4.");
                    throw std::system_error(errno, std::system_category(), "inet_ntop failed converting client IPv4");
                }
                m_logger.log(LogLevel::ERROR, "Server", "Error while trying to convert client IP.");
            } else {

                auto *castedClientAddrIPv6 = reinterpret_cast<struct sockaddr_in6 *>(&cli_addr);

                if (inet_ntop(AF_INET6, &castedClientAddrIPv6->sin6_addr, clientIPArray.data(),
                              sizeof(clientIPArray)) == nullptr) {
                    m_logger.log(LogLevel::ERROR, "Server", "Error while trying to convert client IPv6.");
                    throw std::system_error(errno, std::system_category(), "inet_ntop failed converting client IPv6");
                }
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

        // Check upd activity
        if (activity > 0 && FD_ISSET(m_serverUDPFD, &read_fds)) {
            handleUdpMessage();
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
    if (getaddrinfo(nullptr, std::to_string(m_port).c_str(), &hints, &resultList) != 0) {
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
    if (listen(m_serverTCPFD, MAX_CLIENTS_PERMITTED) < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "Failed to listen on socket.");
        throw std::runtime_error("Failed to listen");
    }

    m_logger.log(LogLevel::INFO, "Server", "Server TCP socket listening on port " + std::to_string(m_port));
    cout << "Server TCP socket listening on port: " << std::to_string(m_port) << " with FD: " << m_serverTCPFD << '\n';
}

void Server::setUDPConfig() {

    struct addrinfo udp_hints {};
    struct addrinfo *udp_result = nullptr;
    udp_hints.ai_family = AF_INET6;     // dual-stack
    udp_hints.ai_socktype = SOCK_DGRAM; // UDP
    udp_hints.ai_flags = AI_PASSIVE;

    // Use another port TCP + 1
    std::string udp_port_str = std::to_string(m_port + 1);
    if (getaddrinfo(nullptr, udp_port_str.c_str(), &udp_hints, &udp_result) != 0) {
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
    m_logger.log(LogLevel::INFO, "Server", "UDP socket listening on port " + udp_port_str);
    cout << "Server UDP socket listening on port: " << udp_port_str << " with FD: " << m_serverUDPFD << '\n';
}

void Server::handleUdpMessage() {

    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);
    std::array<char, BUFFER_SIZE_UDP> buffer{}; // Buffer for message

    ssize_t bytes_read = recvfrom(m_serverUDPFD, buffer.data(), buffer.size() - 1, 0,
                                  reinterpret_cast<struct sockaddr *>(&client_addr), &addr_len);

    if (bytes_read < 0) {
        m_logger.log(LogLevel::ERROR, "Server", "UDP recvfrom error");
        return;
    }
    buffer[bytes_read] = '\0';

    cout << "Received UDP message: " + std::string(buffer.data()) << '\n';
    m_logger.log(LogLevel::INFO, "Server", "Received UDP message: " + std::string(buffer.data()));

    // Process the message UDP here or call a function
    // if necesssary, answer with sendto()
}