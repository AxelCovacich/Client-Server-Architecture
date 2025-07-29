#ifndef ARGS_PARSER_HPP
#define ARGS_PARSER_HPP

#include <optional>
#include <string>
#include <vector>
#define UPPER_PORT_LIMIT 65535
namespace ArgsParser {

struct appConfig {
    int port;
    std::string dbPath;
};

using ParseResult = std::optional<appConfig>;

/**
 * @brief Parses command-line arguments for the server.
 * @param argc The argument count from main.
 * @param argv The argument vector from main.
 * @return An optional containing the port number on success, or an empty optional on failure.
 */
ParseResult parseArguments(const std::vector<std::string> &args);

} // namespace ArgsParser

#endif // ARGS_PARSER_HPP