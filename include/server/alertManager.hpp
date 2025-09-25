#ifndef ALERTMANAGER_HPP
#define ALERTMANAGER_HPP

#include "logger.hpp"
#include "sessionManager.hpp"
#include "udpHandler.hpp"

class AlertManager {

  private:
    Logger &m_logger;
    SessionManager &m_sessionManager;
    UdpHandler &m_udpHandler;

  public:
    /** @brief Constructs a new AlertManager.
     * @param logger A reference to the shared system logger.
     * @param sessionManager A reference to the shared session manager.
     * @param udpHandler A reference to the shared UDP handler for broadcasting alerts.
     */
    AlertManager(Logger &logger, SessionManager &sessionManager, UdpHandler &udpHandler);

    /**
     * @brief Processes an incoming alert message.
     *
     * This function parses the alert message, logs it, and takes appropriate actions
     * such as locking the client account if necessary. It also broadcasts the alert
     * to all connected clients via UDP.
     *
     * @param alertMessage The raw JSON string of the alert message.
     */
    void processAlert(const std::string &alertMessage);
};

#endif // ALERTMANAGER_HPP