#include "config.hpp"
#include <iostream>

Config::Config(const std::vector<std::string> &args) {

    m_configNode = YAML::LoadFile(args[1]);

    m_secretPhrase = m_configNode["security"]["unlock_secret_phrase"].as<std::string>();

    const char *db_path_env = std::getenv("DB_PATH");
    if (db_path_env != nullptr) {
        m_dbPath = std::string(db_path_env);
    } else {
        m_dbPath = m_configNode["database"]["path"].as<std::string>();
    }

    if (args.size() == 3) {
        // Prio 1: CLI
        m_port = std::stoi(args[2]);
    } else {

        const char *portEnv = std::getenv("SERVER_PORT");
        if (portEnv != nullptr) {

            // Prio 2: env variable
            m_port = std::stoi(portEnv);
        } else {

            // Prio 3: yaml file
            m_port = m_configNode["server"]["port"].as<int>();
        }
    }
}

// for testing purposes only
Config::Config(const YAML::Node &node)
    : m_configNode(node) {
}

int Config::getPort() const {
    return m_port;
}

std::string Config::getDbPath() const {
    return m_dbPath;
}

std::string Config::getSecretPhrase() const {
    return m_secretPhrase;
}