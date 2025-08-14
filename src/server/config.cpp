#include "config.hpp"
#include <iostream>

Config::Config(const std::vector<std::string> &args) {

    m_configNode = YAML::LoadFile(args[1]);

    if (!m_configNode["security"] || !m_configNode["security"]["unlock_secret_phrase"] ||
        !m_configNode["security"]["unlock_secret_phrase"].IsScalar()) {
        throw std::runtime_error("Secret phrase is not set in config file");
    }
    m_secretPhrase = m_configNode["security"]["unlock_secret_phrase"].as<std::string>();
    if (m_secretPhrase.empty()) {
        throw std::runtime_error("Secret phrase is not set in config file");
    }

    if (!m_configNode["database"] || !m_configNode["database"]["path"] ||
        !m_configNode["database"]["path"].IsScalar()) {
        throw std::runtime_error("Database path is not set in config file");
    }
    m_dbPath = m_configNode["database"]["path"].as<std::string>();
    if (m_dbPath.empty()) {
        throw std::runtime_error("Database path is not set in config file");
    }

    if (!m_configNode["server"] || !m_configNode["server"]["port"] || !m_configNode["server"]["port"].IsScalar()) {
        throw std::runtime_error("Port in config file is not set or is not a valid number");
    }
    int port = m_configNode["server"]["port"].as<int>();
    if (port <= 0 || port > MAX_PORT) {
        throw std::runtime_error("Port in config file out of valid range");
    }
    m_tcpPort = port;
    m_udpPort = port;

    if (args.size() >= 3) {
        m_tcpPort = std::stoi(args[2]); // at this point args are already validated, no need to check for exceptions
        m_udpPort = m_tcpPort;          // If only one port is provided, use it for both TCP and UDP
    }
    if (args.size() == 4) {
        m_udpPort = std::stoi(args[3]);
    } else if (args.size() < 3) {
        const char *env_port = getenv("SERVER_PORT");
        if (env_port != nullptr) {
            try {
                int env_port_num = std::stoi(env_port);
                if (env_port_num <= 0 || env_port_num > MAX_PORT) {
                    throw std::runtime_error("Invalid port number in environment variable. Out of range.");
                }
                m_tcpPort = env_port_num;
                m_udpPort = env_port_num;
            } catch (const std::exception &) {
                throw std::runtime_error("Environment variable SERVER_PORT is not a valid number");
            }
        }
    }
}

// for testing purposes only
Config::Config(const YAML::Node &node)
    : m_configNode(node) {
}

int Config::getTcpPort() const {
    return m_tcpPort;
}

int Config::getUdpPort() const {
    return m_udpPort;
}

std::string Config::getDbPath() const {
    return m_dbPath;
}

std::string Config::getSecretPhrase() const {
    return m_secretPhrase;
}
