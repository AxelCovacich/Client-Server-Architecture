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
    AlertManager(Logger &logger, SessionManager &sessionManager, UdpHandler &udpHandler);

    void processAlert(const std::string &alertMessage);
};

#endif // ALERTMANAGER_HPP