#include "argsParser.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

namespace ArgsParser {

ParseResult parseArguments(const std::vector<std::string> &args) {
    if (args.size() != 3) {
        cerr << "Usage: " << args[0] << " <port> <database_path" << '\n';
        return std::nullopt;
    }

    try {
        int port = stoi(args[1]); // stoi throws excep if not a number
        const std::string &dbPath = args[2];
        if (port <= 0 || port > UPPER_PORT_LIMIT) {
            cerr << "Error: Port must be between 1 and 65535." << '\n';
            return std::nullopt;
        }
        return appConfig{port, dbPath}; // Success
    } catch (const exception &e) {
        cerr << "Error: Invalid port number provided." << '\n';
        return std::nullopt;
    }
}

} // namespace ArgsParser