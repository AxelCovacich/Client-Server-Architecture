#include "commandProcessor.hpp"
#include "inventory.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

namespace commandProcessor {

commandResult processCommand(const json &request, const std::string &clientId, bool is_in_maintenance,
                             Inventory &inventory) {

    json response;

    //  Validate command exists
    if (!request.contains("command")) {
        response["status"] = "error";
        response["message"] = "Missing 'command' field in request.";
        return {response.dump(), true}; // Keep waiting for valid command
    }

    //  Extract the command
    std::string cmd = request["command"];

    //  Process the command
    if (cmd == "status") {
        response["status"] = "success";
        response["message"] = is_in_maintenance ? "STATUS: MAINTENANCE" : "STATUS: OK";
        return {response.dump(), true};
    }

    if (is_in_maintenance) {
        response["status"] = "error";
        response["message"] = "Server is in maintenance mode.";
        return {response.dump(), true};
    }

    if (cmd == "end") {
        response["status"] = "success";
        response["message"] = "Goodbye!";
        return {response.dump(), false}; // To finish
    }

    if (cmd == "get_inventory") {

        std::string inventoryDataStr = inventory.getInventorySummaryJson(clientId);

        // parse again to correct nesting
        json inventoryData = json::parse(inventoryDataStr);

        response["status"] = "success";
        response["message"] = "Inventory data for client " + clientId + " retrieved.";
        response["data"] = inventoryData;

        return {response.dump(), true};
    }

    if (cmd == "update_stock") {

        try {

            // 1. .at() to add values and check. If a key is not present, catch will attend.
            const json &payload = request.at("payload");
            const std::string category = payload.at("category");
            const std::string item = payload.at("item");
            const int quantity = payload.at("quantity");

            // 2. All good to upload
            // CALL RESPONSABLE FOR UPDATING STOCK HERE
            inventory.updateStock(clientId, category, item, quantity);

            // 3. Success response.
            response["status"] = "success";
            std::string message = "Stock for '";
            message += category + ":" + item;
            message += "' successfully updated to " + std::to_string(quantity) + ".";

            response["message"] = message;
            return {response.dump(), true};

        } catch (const json::exception &e) {
            // 4. If any key is missing, or wrong type this will treat it
            //    (ej: "quantity": "abc")
            cerr << "Invalid payload for update_stock: " << e.what() << '\n';
            response["status"] = "error";
            response["message"] = "Invalid or missing field in update_stock payload.";
            return {response.dump(), true};
        }
    }

    // for unknown command
    response["status"] = "success"; // Is not an error, its just unknown
    response["message"] = "ACK: Unknown command received.";
    return {response.dump(), true};
}
} // namespace commandProcessor