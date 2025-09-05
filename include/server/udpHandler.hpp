#ifndef UDP_HANDLER_HPP
#define UDP_HANDLER_HPP

#include "clientSession.hpp"
#include "eventQueue.hpp"
#include "logger.hpp"
#include "sessionManager.hpp"
#include "trafficReporter.hpp"
#include <array>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/socket.h>

using json = nlohmann::json;

#define BUFFER_SIZE_UDP 256

/**
 * @class UdpHandler
 * @brief Handles processing of incoming UDP datagrams.
 *
 * This class encapsulates the logic for receiving and dispatching UDP messages,
 * primarily for handling client keepalives.
 */
class UdpHandler {
  public:
    /**
     * @brief Constructs a new UdpHandler.
     * @param udpSocketFd The server's listening UDP socket file descriptor.
     * @param logger A reference to the shared server logger.
     * @param sessionManager A reference to the shared session manager.
     * @param trafficReporter A reference to the shared traffic reporter.
     */
    UdpHandler(int udpSocketFd, Logger &logger, SessionManager &sessionManager, TrafficReporter &trafficReporter,
               EventQueue &eventQueue);

    /**
     * @brief The main entry point for processing a UDP message.
     *
     * Called by the Server's main loop when UDP activity is detected. It reads
     * the datagram, parses it, and updates the corresponding client's session.
     */
    void handleMessage();

    void broadcastMessage(const json &alertMessage);

    void setSocketFd(int udpFD) {
        m_udpSocketFd = udpFD;
    }

  private:
    /**
     * @brief Processes a keepalive message from a client.
     * @param request The parsed JSON object from the datagram.
     * @param client_addr The network address of the client who sent the message.
     */
    void handleKeepalive(const json &request, const struct sockaddr_storage &client_addr);

    int m_udpSocketFd;

    EventQueue m_eventQueue;
    Logger &m_logger;
    SessionManager &m_sessionManager;
    TrafficReporter &m_trafficReporter;
};

#endif // UDP_HANDLER_HPP