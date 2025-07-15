
#include "server.hpp"
#include <commandProcessor.hpp>
#include <csignal> // For std::signal
#include <cstring> // For memset()
#include <iostream>
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

Server::Server(int port) : port_(port), server_fd_(-1) {

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

            jthread client_thread(&Server::handleClient, this, newsockfd);
            client_thread.detach();
        }
    }
    cout << "\nShutdown signal received. Server is closing.\n";
    return;
}

void Server::handleClient(int client_socket) {

    cout << "New client connected. Handled by thread: " << this_thread::get_id() << '\n';

    const char *welcome_msg = "Welcome to the C++ Server! Type 'end' to disconnect.\n";
    const char *ack_msg = "ACK: Message received by server.\n";

    if (write(client_socket, welcome_msg, strlen(welcome_msg)) < 0) {
        perror("Writing to client socket");
        close(client_socket);
        return;
    }

    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];

    while (true) {

        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);

        if (bytes_read < 0) {
            perror("Error reading from socket");
            break;
        }

        if (bytes_read == 0) {
            cout << "Client disconnected.\n";
            break;
        }
        buffer[bytes_read] = '\0';

        string client_message(buffer);
        cout << "Thread " << this_thread::get_id() << " received: " << client_message << '\n';

        commandProcessor::commandResult result = commandProcessor::processCommand(client_message);

        if (write(client_socket, result.first.c_str(), result.first.length()) < 0) {

            perror("Error writing response to socket");
            break;
        }

        if (!result.second) {

            cout << "Closing connection based on command.\n";
            break;
        }
    }
    cout << "Closing connection with client from thread: " << this_thread::get_id() << '\n';
    close(client_socket);
}