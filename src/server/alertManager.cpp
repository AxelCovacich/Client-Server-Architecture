#include "alertManager.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace std;

AlertManager::AlertManager(Logger &logger, SessionManager &sessionManager, UdpHandler &udpHandler)
    : m_logger(logger)
    , m_sessionManager(sessionManager)
    , m_udpHandler(udpHandler) {
}

void AlertManager::processAlert(const std::string &alertMessage) {

    try {
        json alertData = json::parse(alertMessage);
        alertData["category"] = "alert"; // Add the category field to the alert data
        // validate fields
        const std::string &type = alertData.at("type");
        const std::string &message = alertData.at("message");

        std::string logMessage = "Received alert from sensor. Type: " + type + ", Message: " + message;
        m_logger.log(LogLevel::WARNING, "AlertManager", logMessage);

        if (alertData.contains("clientId")) {
            const std::string &clientId = alertData.at("clientId");
            if (type == "infection" || type == "enemy_threat") {
                m_sessionManager.lockClient(clientId);
            }
        }

        m_udpHandler.broadcastMessage(alertData);

    } catch (const json::exception &e) {
        cerr << "Error processing alert: " << e.what() << '\n';
        std::string errorLog = "Failed to process incoming alert. Reason: " + std::string(e.what());
        m_logger.log(LogLevel::ERROR, "AlertManager", errorLog);
    }
    // cout << "recieved alert Message: " << alertMessage << '\n';
}