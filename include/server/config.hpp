#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "yaml-cpp/yaml.h"
#include <string>
#include <vector>
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

    int getPort() const;
    std::string getDbPath() const;
    std::string getSecretPhrase() const;

  private:
    YAML::Node m_configNode;
    int m_port;
    std::string m_dbPath;
    std::string m_secretPhrase;
};

#endif // CONFIG_HPP