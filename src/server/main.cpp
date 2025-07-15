#include "argsParser.hpp"
#include "server.hpp"
#include <csignal>
#include <cstdlib> // For atoi
#include <iostream>

using namespace std;

void signal_handler(int signum);

/**
 * @brief Main entry point for the Logistics Server.
 *
 * This executable initializes and runs the TCP server. It requires a port
 * number as a command-line argument. The server will listen indefinitely
 * for client connections until the process is terminated.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments. Expects "./server <port>".
 * @return 0 on successful shutdown, 1 on initialization error.
 */

int main(int argc, char *argv[]) {

    auto parsed_args = ArgsParser::parseArguments(argc, argv);

    if (!parsed_args) {
        return 1;
    }

    int port = *parsed_args;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);

    try {
        Server server(port);
        server.run();
    } catch (const exception &e) {
        cerr << "Cannot start server: " << e.what() << endl;
        return 1;
    }

    return 0;
}