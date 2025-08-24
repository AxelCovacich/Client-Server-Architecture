#include "config.hpp"
#include <iostream>

Config::Config(const std::vector<std::string> &args) {

    std::string configPath;

    if (args.size() == 1) {
        configPath = CONFIG_FILE_PATH;
    } else {
        configPath = args[1];
    }

    m_configNode = YAML::LoadFile(configPath);

    serverConfig(args);
    databaseConfig();
    securityConfig();
    logConfig();
}

// for testing purposes only
Config::Config(const YAML::Node &node) // NOLINT
    : m_configNode(node) {
}

int Config::getTcpPort() const {
    return m_tcpPort;
}

int Config::getUdpPort() const {
    return m_udpPort;
}

int Config::getMaxClients() const {
    return m_maxClients;
}

int Config::getMaxUnixConnections() const {
    return m_maxUnixConnections;
}

int Config::getBlockTimeSeconds() const {
    return m_blockTimeSeconds;
}

std::string Config::getDbPath() const {
    return m_dbPath;
}

std::string Config::getSecretPhrase() const {
    return m_secretPhrase;
}

std::string Config::getLogPath() const {
    return m_logPath;
}

int Config::getMaxLogSize() const {
    return m_maxLogSize;
}

void Config::logConfig() {

    if (!m_configNode["logger"] || !m_configNode["logger"]["max_log_size_mb"] ||
        !m_configNode["logger"]["max_log_size_mb"].IsScalar()) {
        throw std::runtime_error("Max log size in config file is not set or is not a valid number");
    }
    m_maxLogSize = m_configNode["logger"]["max_log_size_mb"].as<int>();
    if (m_maxLogSize <= 0) {
        throw std::runtime_error("Max log size in config file must be a positive greater than zero integer");
    }

    if (!m_configNode["logger"]["log_path"] || !m_configNode["logger"]["log_path"].IsScalar()) {
        throw std::runtime_error("Log path in config file is not set or is not a valid string");
    }
    m_logPath = m_configNode["logger"]["log_path"].as<std::string>();
    if (m_logPath.empty()) {
        throw std::runtime_error("Log path in config file is not set or is empty");
    }
}

void Config::serverConfig(const std::vector<std::string> &args) {

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

    if (!m_configNode["server"]["max_clients"] || !m_configNode["server"]["max_clients"].IsScalar()) {
        throw std::runtime_error("Max clients in config file is not set or is not a valid number");
    }
    m_maxClients = m_configNode["server"]["max_clients"].as<int>();
    if (m_maxClients <= 0) {
        throw std::runtime_error("Max clients in config file must be a positive greater than zero integer");
    }

    if (!m_configNode["server"]["max_unix_connections"] || !m_configNode["server"]["max_unix_connections"].IsScalar()) {
        throw std::runtime_error("Max unix connections in config file is not set or is not a valid number");
    }
    m_maxUnixConnections = m_configNode["server"]["max_unix_connections"].as<int>();
    if (m_maxUnixConnections <= 0) {
        throw std::runtime_error("Max unix connections in config file must be a positive greater than zero integer");
    }
}

void Config::databaseConfig() {
    if (!m_configNode["database"] || !m_configNode["database"]["path"] ||
        !m_configNode["database"]["path"].IsScalar()) {
        throw std::runtime_error("Database path is not set in config file");
    }
    m_dbPath = m_configNode["database"]["path"].as<std::string>();
    if (m_dbPath.empty()) {
        throw std::runtime_error("Database path is not set in config file");
    }
}

void Config::securityConfig() {

    if (!m_configNode["security"] || !m_configNode["security"]["unlock_secret_phrase"] ||
        !m_configNode["security"]["unlock_secret_phrase"].IsScalar()) {
        throw std::runtime_error("Secret phrase is not set in config file");
    }
    m_secretPhrase = m_configNode["security"]["unlock_secret_phrase"].as<std::string>();
    if (m_secretPhrase.empty()) {
        throw std::runtime_error("Secret phrase is not set in config file");
    }

    if (!m_configNode["security"]["block_time_seconds"] || !m_configNode["security"]["block_time_seconds"].IsScalar()) {
        throw std::runtime_error("Block time seconds is not set in config file");
    }
    m_blockTimeSeconds = m_configNode["security"]["block_time_seconds"].as<int>();
    if (m_blockTimeSeconds <= 0) {
        throw std::runtime_error("Block time seconds must be a positive greater than zero integer");
    }
}
