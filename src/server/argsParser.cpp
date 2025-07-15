#include "argsParser.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

namespace ArgsParser {

ParseResult parseArguments(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        return std::nullopt;
    }

    try {
        int port = stoi(argv[1]); // stoi throws excep if not a number

        if (port <= 0 || port > 65535) {
            cerr << "Error: Port must be between 1 and 65535." << endl;
            return std::nullopt;
        }
        return port; // Éxito
    } catch (const exception &e) {
        cerr << "Error: Invalid port number provided." << endl;
        return std::nullopt;
    }
}

} // namespace ArgsParser