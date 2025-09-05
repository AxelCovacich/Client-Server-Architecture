#include "udpHandler.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unordered_map>

using namespace std;

UdpHandler::UdpHandler(int udpSocketFd, Logger &logger, SessionManager &sessionManager,
                       TrafficReporter &trafficReporter, EventQueue &eventQueue)
    : m_udpSocketFd(udpSocketFd)
    , m_logger(logger)
    , m_sessionManager(sessionManager)
    , m_trafficReporter(trafficReporter)
    , m_eventQueue(eventQueue) {
}
void UdpHandler::handleMessage() {

    struct sockaddr_storage client_addr {};
    socklen_t addr_len = sizeof(client_addr);
    std::array<char, BUFFER_SIZE_UDP> buffer{}; // Buffer for message

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    ssize_t bytes_read = recvfrom(m_udpSocketFd, buffer.data(), buffer.size() - 1, 0,
                                  reinterpret_cast<struct sockaddr *>(&client_addr), &addr_len); // NOLINT

    if (bytes_read < 0) {
        perror("recvfrom");
        // cout << "Error receiving UDP message.\n";
        m_logger.log(LogLevel::ERROR, "UdpHandler", "UDP recvfrom error");
        m_trafficReporter.incrementError("udp", "rx");
        return;
    }
    buffer[bytes_read] = '\0'; // NOLINT bytes_read will always be at max buffer.size - 1

    // cout << "Received UDP message: " + std::string(buffer.data()) << '\n';
    m_logger.log(LogLevel::INFO, "UdpHandler", "Received UDP message: " + std::string(buffer.data()));
    m_trafficReporter.incrementMessage("udp", "rx");

    json request = json::parse(std::string(buffer.data()), nullptr, false);
    if (request.is_discarded()) {
        m_logger.log(LogLevel::WARNING, "UdpHandler", "Receiveed invalid JSON via UDP.");
        return;
    }

    std::string cmd = request.value("command", "unknown"); // search for 'command', if not, default is unknown

    if (cmd == "keepalive") {
        // cout << "Received keepalive message from client.\n";
        handleKeepalive(request, client_addr);
    } else {
        m_logger.log(LogLevel::WARNING, "UdpHandler", "Received unknown UDP command: " + cmd);
    }
}

void UdpHandler::handleKeepalive(const json &request, const struct sockaddr_storage &client_addr) {
    try {
        std::string clientId = request.at("clientId");

        std::shared_ptr<clientSession> session = m_sessionManager.getSession(clientId);

        if (session) {
            session->setUdpAddress(client_addr);
            // cout<<"DEBUG: Updated UDP address for client " << clientId << '\n';
            m_logger.log(LogLevel::DEBUG, "UdpHandler", "Updated UDP address for client " + clientId);

            json pongMessage;
            pongMessage["category"] = "keepalive";
            pongMessage["status"] = "success";
            pongMessage["message"] = "PONG: Server received keepalive from client and connection is active.";
            std::string pongString = pongMessage.dump();

            socklen_t addr_len =
                (client_addr.ss_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

            size_t sent = sendto(m_udpSocketFd, pongString.c_str(), pongString.length(), 0,
                                 reinterpret_cast<const struct sockaddr *>(&client_addr), addr_len); // NOLINT
            if (sent < 0) {
                m_logger.log(LogLevel::ERROR, "UdpHandler", "Error sending PONG message to client " + clientId);
                m_trafficReporter.incrementError("udp", "tx");
            } else {
                m_trafficReporter.incrementMessage("udp", "tx");
                m_logger.log(LogLevel::INFO, "UdpHandler", "Sent PONG message to client " + clientId);
            }
        } else {
            // cout << "DEBUG: No session found for keepalive message from client " << clientId << '\n';
            m_logger.log(LogLevel::ERROR, "UdpHandler",
                         "Couldn't find session for keepalive message from client " + clientId);
        }

    } catch (const json::exception &e) {
        cerr << "JSON parsing error in keepalive message: " << e.what() << '\n';
        m_logger.log(LogLevel::WARNING, "UdpHandler", "Malformed keepalive message received.");
    }
}

void UdpHandler::broadcastMessage(const json &alertMessage) {

    std::vector<struct sockaddr_storage> addresses = m_sessionManager.getActiveUdpAddresses();
    std::string alert_str = alertMessage.dump();

    for (const auto &address : addresses) {
        socklen_t addr_len = (address.ss_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

        ssize_t sent = sendto(m_udpSocketFd, alert_str.c_str(), alert_str.length(), 0,
                              reinterpret_cast<const struct sockaddr *>(&address), addr_len); // NOLINT

        if (sent < 0) {
            m_logger.log(LogLevel::ERROR, "UdpHandler", "Error broadcasting alert message");
            m_trafficReporter.incrementError("udp", "tx");
        } else {
            m_trafficReporter.incrementMessage("udp", "tx");
        }
    }
    m_logger.log(LogLevel::INFO, "UdpHandler", "Broadcasted alert message to all connected clients: " + alert_str);

    std::unordered_set<std::string> offlineUsers = m_sessionManager.getOfflineUsers();
    for (const auto &user : offlineUsers) {
        // queue messages for later delivery
        m_eventQueue.pushEvent(user, Event{EventType::NOTIFICATION, alert_str});
        m_logger.log(LogLevel::INFO, "UdpHandler",
                     "User " + user + " is offline, alert has been queued for later delivery.");
    }
}
