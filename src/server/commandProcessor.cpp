#include "commandProcessor.hpp"
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;
using namespace std;

namespace commandProcessor {

static commandResult handleStatusCommand(const json &request, bool IsInMaintenance, json &response);
static commandResult handleEndCommand(const json &request, json &response, Logger &logger, const std::string &clientId);
static commandResult handleGetInventoryCommand(const json &request, const std::string &clientId, Inventory &inventory,
                                               Logger &logger, json &response, TrafficReporter &trafficReporter);
static commandResult handleUpdateStockCommand(const json &request, const std::string &clientId, Inventory &inventory,
                                              Logger &logger, json &response, TrafficReporter &trafficReporter);
static commandResult handleGetStockCommand(const json &request, const std::string &clientId, Inventory &inventory,
                                           Logger &logger, json &response, TrafficReporter &trafficReporter);
static commandResult handleGetHistoryCommand(const json &request, const std::string &clientId, Storage &storage,
                                             Logger &logger, json &response);
static commandResult handleUnlockClientCommand(const json &request, const std::string &clientId,
                                               SessionManager &sessionManager, const Config &config, Logger &logger,
                                               json &response, TrafficReporter &trafficReporter);

commandResult processCommand(const json &request, const std::string &clientId, bool isInMaintenance,
                             Inventory &inventory, Logger &logger, Storage &storage, SessionManager &sessionManager,
                             const Config &config, TrafficReporter &trafficReporter) {

    json response;

    //  Validate command exists
    if (!request.contains("command")) {
        logger.log(LogLevel::WARNING, "CommandProcessor",
                   "Request from client " + clientId + " is missing 'command' field.", clientId);
        response["status"] = "error";
        response["message"] = "Missing 'command' field in request.";
        trafficReporter.incrementError("tcp", "rx");
        return {response.dump(), true}; // Keep waiting for valid command
    }

    //  Extract the command
    std::string cmd = request["command"];

    // Log the command to process
    // logger.log(LogLevel::DEBUG, "CommandProcessor", "Processing '" + cmd + "' command for client " + clientId,
    // clientId);

    //  Process the command
    if (cmd == "status") {
        return handleStatusCommand(request, isInMaintenance, response);
    }

    if (isInMaintenance) {
        response["status"] = "error";
        response["message"] = "Server is in maintenance mode.";
        return {response.dump(), true};
    }

    if (cmd == "end") {

        return handleEndCommand(request, response, logger, clientId);
    }

    if (cmd == "get_inventory") {

        return handleGetInventoryCommand(request, clientId, inventory, logger, response, trafficReporter);
    }

    if (cmd == "update_stock") {

        return handleUpdateStockCommand(request, clientId, inventory, logger, response, trafficReporter);
    }

    if (cmd == "get_stock") {
        return handleGetStockCommand(request, clientId, inventory, logger, response, trafficReporter);
    }

    if (cmd == "get_history") {
        return handleGetHistoryCommand(request, clientId, storage, logger, response);
    }

    if (cmd == "unlock_client") {
        return handleUnlockClientCommand(request, clientId, sessionManager, config, logger, response, trafficReporter);
    }

    // for unknown command
    response["status"] = "success"; // Is not an error, its just unknown
    response["message"] = "ACK: Unknown command received.";
    logger.log(LogLevel::WARNING, "CommandProcessor", "Received unknown command '" + cmd + "' from client " + clientId,
               clientId);
    return {response.dump(), true};
}

static commandResult handleStatusCommand(const json &request, bool isInMaintenance, json &response) {
    response["status"] = "success";
    response["message"] = isInMaintenance ? "STATUS: MAINTENANCE" : "STATUS: OK";
    return {response.dump(), true};
}

static commandResult handleEndCommand(const json &request, json &response, Logger &logger,
                                      const std::string &clientId) {
    response["status"] = "success";
    response["message"] = "Goodbye!";
    logger.log(LogLevel::INFO, "CommandProcessor", "Requested disconnection via end command for client " + clientId,
               clientId);
    return {response.dump(), false}; // To finish
}

static commandResult handleGetInventoryCommand(const json &request, const std::string &clientId, Inventory &inventory,
                                               Logger &logger, json &response, TrafficReporter &trafficReporter) {

    auto inventoryMap = inventory.getInventorySummary(clientId);

    if (inventoryMap.has_value()) {
        json inventoryData = *inventoryMap;
        response["status"] = "success";
        response["message"] = "Inventory data for client " + clientId + " retrieved.";
        response["data"] = inventoryData;
    } else {
        response["status"] = "error";
        trafficReporter.incrementError("tcp", "rx");
        response["message"] = "Inventory data for client " + clientId + " is empty.";
    }
    return {response.dump(), true};
}

static commandResult handleUpdateStockCommand(const json &request, const std::string &clientId, Inventory &inventory,
                                              Logger &logger, json &response, TrafficReporter &trafficReporter) {

    try {

        // 1. .at() to add values and check. If a key is not present, catch will attend.
        const json &payload = request.at("payload");
        const std::string category = payload.at("category");
        const std::string item = payload.at("item");
        const int quantity = payload.at("quantity");
        auto result = inventory.updateStock(clientId, category, item, quantity);

        if (result.success) {

            response["status"] = "success";
        } else {

            response["status"] = "error";
            trafficReporter.incrementError("tcp", "rx");
        }
        response["message"] = result.message;
        return {response.dump(), true};

    } catch (const json::exception &e) {
        // If any key is missing, or wrong type this will treat it
        //    (ej: "quantity": "abc")
        // cerr << "Invalid payload for update_stock: " << e.what() << '\n';
        response["status"] = "error";
        response["message"] = "Invalid or missing field in update_stock payload.";
        trafficReporter.incrementError("tcp", "rx");
        logger.log(LogLevel::WARNING, "CommandProcessor", "Invalid or missing field in update_stock payload.",
                   clientId);

        return {response.dump(), true};
    }
}

static commandResult handleGetStockCommand(const json &request, const std::string &clientId, Inventory &inventory,
                                           Logger &logger, json &response, TrafficReporter &trafficReporter) {

    try {
        const json &payload = request.at("payload");
        const std::string category = payload.at("category");
        const std::string item = payload.at("item");

        std::optional<int> result = inventory.getStock(clientId, category, item);

        if (result.has_value()) {
            response["status"] = "success";
            std::string message = "Stock data retrieved successfully.";
            response["message"] = message;

            json data_payload;
            data_payload["category"] = category;
            data_payload["item"] = item;
            data_payload["quantity"] = result.value();

            response["data"] = data_payload;
            return {response.dump(), true};
        }
        response["status"] = "error";
        response["message"] = "Item not found for the specified client/category.";
        trafficReporter.incrementError("tcp", "rx");
        return {response.dump(), true};

    } catch (const json::exception &e) {

        // cerr << "Invalid payload for get_stock: " << e.what() << '\n';
        response["status"] = "error";
        response["message"] = "Invalid or missing field in get_stock payload.";
        trafficReporter.incrementError("tcp", "rx");
        logger.log(LogLevel::WARNING, "CommandProcessor", "Invalid or missing field in get_stock payload.", clientId);
        return {response.dump(), true};
    }
}

static commandResult handleGetHistoryCommand(const json &request, const std::string &clientId, Storage &storage,
                                             Logger &logger, json &response) {

    std::vector<LogEntry> history = storage.getInventoryHistoryTransaction(clientId);

    response["status"] = "success";
    response["message"] = "Inventory history retrieved.";

    json historyArray = json::array(); // create an empty json array to store temporarly store the logs

    for (const auto &entry : history) {
        json logEntry;
        logEntry["timestamp"] = entry.timestamp;
        logEntry["message"] = entry.message;
        logEntry["component"] = entry.component;
        logEntry["level"] = entry.level;
        historyArray.push_back(logEntry); // add the json object logentry with the contents of the log to the array
    }

    response["data"] = historyArray; // now add to the data field the entire array with all the logs captured in the
                                     // loop if empty(no logs found), it just returns an empty field []
    return {response.dump(), true};
}

static commandResult handleUnlockClientCommand(const json &request, const std::string &clientId,
                                               SessionManager &sessionManager, const Config &config, Logger &logger,
                                               json &response, TrafficReporter &trafficReporter) {
    if (clientId == "admin") {

        try {
            const json &payload = request.at("payload");
            const std::string secretPhrase = payload.at("secret_phrase");
            const std::string clientToUnlock = payload.at("client_to_unlock");
            const std::string configSecretPhrase = config.getSecretPhrase();

            if (secretPhrase == configSecretPhrase) {
                bool success = sessionManager.unlockClient(clientToUnlock);
                if (success) {
                    response["status"] = "success";
                    response["message"] = "Client `" + clientToUnlock + "` successfully unlocked.";
                    return {response.dump(), true};
                }
                response["status"] = "error";
                response["message"] =
                    "Couldn't unlock Client `" + clientToUnlock + "`. Client didn't found or misspelled.";
                trafficReporter.incrementError("tcp", "rx");
                logger.log(LogLevel::WARNING, "CommandProcessor",
                           "Couldn't unlock Client `" + clientToUnlock + "`. Client didn't found or misspelled.",
                           clientId);
                return {response.dump(), true};
            }
            response["status"] = "error";
            response["message"] = "Wrong secret phrase. Try again.";
            trafficReporter.incrementError("tcp", "rx");
            logger.log(LogLevel::WARNING, "CommandProcessor", "Wrong secret phrase provided for unlock_client command.",
                       clientId);
            return {response.dump(), true};
        } catch (const json::exception &e) {
            response["status"] = "error";
            response["message"] = "Invalid or missing field in unlock_client payload.";
            trafficReporter.incrementError("tcp", "rx");
            logger.log(LogLevel::WARNING, "CommandProcessor", "Invalid or missing field in unlock_client payload.",
                       clientId);
            return {response.dump(), true};
        }
    } else {
        response["status"] = "error";
        response["message"] = "Command only available for admin user.";
        trafficReporter.incrementError("tcp", "rx");
        logger.log(LogLevel::WARNING, "CommandProcessor", "Trying to execute admin commands with a invalid user.",
                   clientId);
        return {response.dump(), true};
    }
}

} // namespace commandProcessor