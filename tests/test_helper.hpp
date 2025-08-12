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
        database:
            path: ":memory:"
        security:
            unlock_secret_phrase: "dummy_phrase"
    )");
    return Config(dummyNode);
}

inline void createTempYamlFile(const std::string &content) {
    std::ofstream file("./temp_config.yaml");
    file << content;
    file.close();
}