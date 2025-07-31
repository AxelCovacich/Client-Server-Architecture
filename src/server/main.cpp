#include "argsParser.hpp"
#include "authenticator.hpp"
#include "inventory.hpp"
#include "server.hpp"
#include "storage.hpp"
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

    const std::vector<std::string> args(argv, argv + argc); // vector that contains all the elements of the command line
    std::unique_ptr<Server> server = nullptr; // smart pointer to server lives in the stack to controll the server
                                              // object

    auto parsedArgs = ArgsParser::parseArguments(args);
    if (!parsedArgs) {
        return 1;
    }

    int port = parsedArgs->port;
    std::string dbPath = parsedArgs->dbPath;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);

    try {

        SystemClock clock;
        Storage storage(dbPath);
        storage.initializeSchema();
        Logger logger(storage, clock, std::cerr);
        Authenticator authenticator(storage, clock, logger);
        Inventory inventory(storage, logger);

        logger.log(LogLevel::INFO, "Main", "Core services initialized.");

        // making a server object on the heap, controlled by smart pointer server
        server = std::make_unique<Server>(port, inventory, authenticator, logger, storage);
        logger.log(LogLevel::INFO, "Main", "Server successfully started. Running loop...");

        server->run();

    } catch (const exception &e) {
        std::cerr << "CRITICAL SERVER FAILURE: \n" << e.what();
        return 1;
    }

    return 0;
}