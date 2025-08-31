#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "yaml-cpp/yaml.h"
#include <string>
#include <vector>

#define MAX_PORT 65535
#define CONFIG_FILE_PATH "./etc/config.yaml"
/**
 * @class Config
 * @brief Manages loading and accessing server configuration from a YAML file.
 */
class Config {
  public:
    /**
     * @brief Constructs a Config object by loading and parsing a YAML file.
     * @param config_path The file path to the configuration YAML file.
     * @throw YAML::Exception if the file cannot be parsed.
     */
    explicit Config(const std::vector<std::string> &args);

    /**
     * @brief (For testing only) Constructs a Config object from an existing YAML node.
     */
    explicit Config(const YAML::Node &node);

    int getMaxLogSize() const;
    std::string getLogPath() const;
    int getMaxClients() const;
    int getMaxUnixConnections() const;
    int getBlockTimeSeconds() const;
    int getTcpPort() const;
    int getUdpPort() const;
    std::string getDbPath() const;
    std::string getSecretPhrase() const;
    std::string getMetricHostPort() const;

    void logConfig();
    void serverConfig(const std::vector<std::string> &args);
    void databaseConfig();
    void securityConfig();

  private:
    YAML::Node m_configNode;
    int m_maxLogSize;
    int m_maxClients;
    int m_maxUnixConnections;
    int m_blockTimeSeconds;
    int m_tcpPort;
    int m_udpPort;
    std::string m_metricHostPort;
    std::string m_logPath;
    std::string m_dbPath;
    std::string m_secretPhrase;
};

#endif // CONFIG_HPP