#ifndef COMMAND_PROCESSOR_HPP
#define COMMAND_PROCESSOR_HPP

#include <string>
#include <utility>

/**
 * @namespace CommandProcessor
 * @brief Encapsulates the server's application-level command processing logic.
 *
 * This namespace contains functions responsible for parsing raw client commands
 * and translating them into server actions and responses.
 */
namespace commandProcessor {

using commandResult = std::pair<std::string, bool>;

/**
 * @brief Processes a raw command string from a client.
 * @param command The command string received from the client.
 * @return A commandResult pair containing the response string and a boolean
 * indicating if the session should continue.
 */
commandResult processCommand(const std::string &command);

} // namespace commandProcessor

#endif // COMMAND_PROCESSOR_HPP