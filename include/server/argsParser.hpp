#ifndef ARGS_PARSER_HPP
#define ARGS_PARSER_HPP

#include <optional>

namespace ArgsParser {

using ParseResult = std::optional<int>;

/**
 * @brief Parses command-line arguments for the server.
 * @param argc The argument count from main.
 * @param argv The argument vector from main.
 * @return An optional containing the port number on success, or an empty optional on failure.
 */
ParseResult parseArguments(int argc, char *argv[]);

} // namespace ArgsParser

#endif // ARGS_PARSER_HPP