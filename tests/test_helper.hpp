// mock config por tests

#include "config.hpp"
#include <fstream>
#include <string>
#include <yaml-cpp/yaml.h>

// default values for tests that need a config object to create their own class object
inline Config createDummyConfig() {
    YAML::Node dummyNode = YAML::Load(R"(
        server:
            port: 8080
            max_clients: 10
            max_unix_connections: 5
            metric_host_port: "localhost:8081"
        logger:
            max_log_size_mb: 10
            log_path: "./var/logs/server.log"
        database:
            path: ":memory:"
        security:
            unlock_secret_phrase: "dummy_phrase"
            block_time_seconds: 900
    )");
    Config cfg(dummyNode);
    cfg.securityConfig();
    cfg.databaseConfig();
    cfg.serverConfig({});
    cfg.logConfig();
    return cfg;
}

inline void createTempYamlFile(const std::string &content) {
    std::ofstream file("./temp_config.yaml");
    file << content;
    file.close();
}