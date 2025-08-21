#include "config.hpp"
#include "configValidator.hpp"
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

    if (!ConfigValidator::validateArguments(args)) {
        return 1;
    }

    std::unique_ptr<Server> server = nullptr; // smart pointer to server lives in the stack to control the server
                                              // object
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);

    try {

        Config config(args);
        SystemClock clock;
        Storage storage(config.getDbPath());
        storage.initializeSchema();
        Logger logger(storage, clock, std::cerr);
        logger.openLogFile(LOG_PATH);
        logger.log(LogLevel::INFO, "Main", "Core services initialized.");

        // making a server object on the heap, controlled by smart pointer server
        server = std::make_unique<Server>(config, clock, storage, logger);
        logger.log(LogLevel::INFO, "Main", "Server successfully started. Running loop...");

        server->run();

    } catch (const exception &e) {
        std::cerr << "CRITICAL SERVER FAILURE: \n" << e.what();
        return 1;
    }

    return 0;
}