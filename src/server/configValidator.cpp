#include "configValidator.hpp"
#include <filesystem>
#include <iostream>

bool ConfigValidator::validateArguments(const std::vector<std::string> &args) {
    if (args.size() < 2 || args.size() > 4) {
        std::cerr << "Usage: " << args[0] << " <path_to_config.yaml> [optional_tcp_port] [optional_udp_port]\n";
        return false;
    }

    const std::string &configPath = args[1];

    if (!std::filesystem::exists(configPath)) {
        std::cerr << "Error: Configuration file not found at path: " << configPath << '\n';
        return false;
    }

    if (args.size() >= 3) {
        try {
            int tcpPort = stoi(args[2]); // stoi throws excep if not a number
            if (tcpPort <= 0 || tcpPort > UPPER_PORT_LIMIT) {
                std::cerr << "Error: TCP port must be between 1 and 65535." << '\n';
                return false;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error: Invalid port number provided." << '\n';
            return false;
        }
    }
    // Validate UDP port if provided
    if (args.size() == 4) {
        try {
            int udp_port = std::stoi(args[3]);
            if (udp_port <= 0 || udp_port > UPPER_PORT_LIMIT) {
                std::cerr << "Error: UDP port must be between 1 and 65535." << '\n';
                return false;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error: Invalid UDP port number provided." << '\n';
            return false;
        }
    }

    return true;
}