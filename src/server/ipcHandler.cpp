#include "ipcHandler.hpp"
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
    bytes_read = read(acceptedSocketFd, buffer.data(), buffer.size() - 1);

    if (bytes_read <= 0) {
        perror("Error reading from unix socket");
        m_logger.log(LogLevel::ERROR, "IpcHandler", "IPC read error");
        m_trafficReporter.incrementError("ipc", "rx");
        return false;
    }

    buffer.at(bytes_read) = '\0';

    std::string alertMessage(buffer.data());

    m_alertManager.processAlert(alertMessage); // Process the alert message
    close(acceptedSocketFd);                   // Close the socket after processing the message
    m_logger.log(LogLevel::INFO, "IpcHandler", "UNIX message processed successfully");
    m_trafficReporter.incrementMessage("ipc", "rx");
    return true;
}
