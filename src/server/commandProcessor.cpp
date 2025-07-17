#include "commandProcessor.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

namespace commandProcessor {

commandResult processCommand(const std::string &command_string, bool is_in_maintenance) {

    json response;
    try {
        // 1. Parse the string to json object
        json request = json::parse(command_string);

        // 2. Validate command exists
        if (!request.contains("command")) {
            response["status"] = "error";
            response["message"] = "Missing 'command' field in request.";
            return {response.dump(), true}; // Keep waiting for valid command
        }

        // 3. Extract the command
        std::string cmd = request["command"];

        // 4. Process the command
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

        if (cmd == "update_stock") {

            try {

                // 1. .at() to add values and check. If a key is not present, catch will attend.
                const json &payload = request.at("payload");
                const std::string category = payload.at("category");
                const std::string item = payload.at("item");
                const int quantity = payload.at("quantity");

                // 2. All good to upload
                // CALL RESPONSABLE FOR UPDATING STOCK HERE

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

    } catch (const json::parse_error &e) {
        // if string is not a valid JSON
        std::cerr << "JSON Parse Error: " << e.what() << '\n';
        response["status"] = "error";
        response["message"] = "Invalid JSON format.";
        return {response.dump(), true}; // continue with the conection
    }
}

} // namespace commandProcessor