
#include "server.hpp"
#include "clientSession.hpp"
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
volatile sig_atomic_t g_shutdown_flag = 0;

/**
 * @brief Signal handler function to flag graceful shutdown.
 * @param signum The signal number received.
 */
void signal_handler(int signum) {
    g_shutdown_flag = 1;
}

Server::Server(int port)
    : port_(port)
    , server_fd_(-1) {

    setupServer();
}

Server::~Server() {
    if (server_fd_ != -1) {
        cout << "Closing server socket.\n";
        close(server_fd_);
    }
}

void Server::setupServer() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw runtime_error("Failed to create socket");
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        throw runtime_error("Failed to bind to port");
    }

    if (listen(server_fd_, 5) < 0) {
        throw runtime_error("Failed to listen on socket");
    }

    cout << "Server listening on port: " << port_ << '\n';
}

void Server::run() {
    fd_set read_fds;
    struct timeval tv;

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    while (!g_shutdown_flag) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd_, &read_fds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        // use selecto to wait for 1 sec or a clients connects and then go for another iteration
        int activity = select(server_fd_ + 1, &read_fds, NULL, NULL, &tv);

        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        if (activity > 0 && FD_ISSET(server_fd_, &read_fds)) {
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            int newsockfd = accept(server_fd_, (struct sockaddr *)&cli_addr, &clilen);

            if (newsockfd < 0) {
                perror("Error on accept");
                continue;
            }

            // this creates a new clientSession object in dinamic memory (new). It wrappes it in a shared_ptr and
            // returns no need to manually delete, local variable session finish and deletes its own ptr. copies of
            // shared_ptr are used by threads independently of local variable session. When all instances(threads)
            // complete their jobs and there is no more copies of the object clientSession via shared_ptr, memory will
            // be freed automaticly.

            auto session = std::make_shared<clientSession>(newsockfd, m_inventory, m_authenticator);
            jthread client_thread(&clientSession::run, session);
            client_thread.detach();
        }
    }
    cout << "\nShutdown signal received. Server is closing.\n";
    return;
}
