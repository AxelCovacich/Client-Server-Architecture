#include "ipcHandler.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

IpcHandler::IpcHandler(Logger &logger, AlertManager &alertManager, TrafficReporter &trafficReporter)
    : m_logger(logger)
    , m_alertManager(alertManager)
    , m_trafficReporter(trafficReporter) {
}

bool IpcHandler::handleConnection(int acceptedSocketFd) {
    ssize_t bytes_read = -1;
    std::array<char, BUFFER_SIZE> buffer{};

    buffer.fill('\0'); // fill the buffer with 0 Just like memset

    // Retry read if interrupted by signal
    // do {
    // bytes_read = read(acceptedSocketFd, buffer.data(), buffer.size() - 1);
    //} while (bytes_read < 0 && errno == EINTR);
    bytes_read = read(acceptedSocketFd, buffer.data(), buffer.size() - 1);
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available now on non-blocking socket
            std::cout << "DEBUG: No data available on IPC socket FD: " << acceptedSocketFd << "\n";
            return true;
        }
        // Real error
        perror("Error reading from unix socket");
        std::cout << "DEBUG: IPC read error on FD: " << acceptedSocketFd << " With errno: " << errno << "\n";
        m_logger.log(LogLevel::ERROR, "IpcHandler", "IPC read error");
        m_trafficReporter.incrementError("ipc", "rx");
        close(acceptedSocketFd);
        return false;
    }
    if (bytes_read == 0) {
        // closed connection
        close(acceptedSocketFd);
        return false;
    }

    buffer.at(bytes_read) = '\0';

    std::string alertMessage(buffer.data(), static_cast<size_t>(bytes_read));

    m_alertManager.processAlert(alertMessage); // Process the alert message
    close(acceptedSocketFd);                   // Close the socket after processing the message
    m_logger.log(LogLevel::INFO, "IpcHandler", "UNIX message processed successfully");
    m_trafficReporter.incrementMessage("ipc", "rx");
    return true;
}
