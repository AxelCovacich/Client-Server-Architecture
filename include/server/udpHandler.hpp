#ifndef UDP_HANDLER_HPP
#define UDP_HANDLER_HPP

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
     * @param eventQueue A reference to the shared event queue.
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

    /**
     * @brief Broadcasts an alert message to all connected clients via UDP.
     * @param alertMessage The JSON object containing the alert details.
     */
    void broadcastMessage(const json &alertMessage);

    /**
     * @brief Sets the UDP socket file descriptor.
     * @param udpFD The file descriptor for the UDP socket.
     */
    void setSocketFd(int udpFD) {
        m_udpSocketFd = udpFD;
    }

    /**
     * @brief Sends a message to a specific client via UDP.
     * @param clientID The unique identifier of the client.
     * @param message The message to send.
     * @param client_addr The network address of the client.
     */
    void sendMessageToClient(const std::string &clientID, const std::string &message,
                             const struct sockaddr_storage &client_addr);

  private:
    /**
     * @brief Processes a keepalive message from a client.
     * @param request The parsed JSON object from the datagram.
     * @param client_addr The network address of the client who sent the message.
     */
    void handleKeepalive(const json &request, const struct sockaddr_storage &client_addr);

    int m_udpSocketFd;

    EventQueue &m_eventQueue;
    Logger &m_logger;
    SessionManager &m_sessionManager;
    TrafficReporter &m_trafficReporter;
};

#endif // UDP_HANDLER_HPP