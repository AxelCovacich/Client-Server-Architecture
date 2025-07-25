#include "argsParser.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

namespace ArgsParser {

ParseResult parseArguments(const std::vector<std::string> &args) {
    if (args.size() != 2) {
        cerr << "Usage: " << args[0] << " <port>" << '\n';
        return std::nullopt;
    }

    try {
        int port = stoi(args[1]); // stoi throws excep if not a number

        if (port <= 0 || port > UPPER_PORT_LIMIT) {
            cerr << "Error: Port must be between 1 and 65535." << '\n';
            return std::nullopt;
        }
        return port; // Success
    } catch (const exception &e) {
        cerr << "Error: Invalid port number provided." << '\n';
        return std::nullopt;
    }
}

} // namespace ArgsParser