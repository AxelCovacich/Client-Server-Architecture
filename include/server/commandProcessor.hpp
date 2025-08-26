#ifndef COMMAND_PROCESSOR_HPP
#define COMMAND_PROCESSOR_HPP

#include "config.hpp"
#include "inventory.hpp"
#include "logger.hpp"
#include "sessionManager.hpp"
#include "storage.hpp"
#include "trafficReporter.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

using json = nlohmann::json;

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
 * @brief Processes a validated JSON request from an authenticated client.
 *
 * This function acts as the central dispatcher for all application-level commands.
 * It takes a parsed JSON object, validates its command and payload, and calls the
 * appropriate business logic modules (Inventory, etc.) to execute the request.
 *
 * @param request The parsed and syntactically valid JSON request object.
 * @param clientId The unique identifier of the authenticated client making the request.
 * @param is_in_maintenance A boolean indicating if the server is in maintenance mode.
 * @param inventory A reference to the server's shared Inventory module.
 * @param logger A reference to the server's shared Logger module.
 * @param storage A reference to the server's shared Storage module.
 * @return A CommandResult pair containing the JSON string response and a boolean
 * indicating if the session should continue.
 */
commandResult processCommand(const json &request, const std::string &clientId, bool is_in_maintenance,
                             Inventory &inventory, Logger &logger, Storage &storage, SessionManager &session,
                             const Config &config, TrafficReporter &trafficReporter);

} // namespace commandProcessor

#endif // COMMAND_PROCESSOR_HPP