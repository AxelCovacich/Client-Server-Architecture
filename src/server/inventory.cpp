#include "inventory.hpp"
#include <map>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

bool Inventory::updateStock(const std::string &clientId, const std::string &category, const std::string &item,
                            int quantity) {

    std::lock_guard<std::mutex> lock(m_mutex);

    if (quantity < 0) {
        printf("Error updating stock.Quantity cannot be negative");
        return false;
    }

    m_inventories[clientId][category][item] = quantity;
    return true;

    // m_storage.saveStock(clientId, category, item, quantity); // future call
    // m_log.logStockUpdate(clientID.....)
}

int Inventory::getStock(const std::string &clientId, const std::string &category, const std::string &item) const {

    // call for mutex, if its free, just execute, if not wait for mutex to release. Unlocks automaticly by finishing the
    // exec of this function
    std::lock_guard<std::mutex> lock(m_mutex);

    auto client_it = m_inventories.find(clientId);
    if (client_it == m_inventories.end()) {
        return 0; // could't find inventory for client
    }

    auto category_it = client_it->second.find(category);
    if (category_it == client_it->second.end()) {
        return 0; // category doesn't exists
    }

    auto item_it = category_it->second.find(item);
    if (item_it == category_it->second.end()) {
        return 0; // no item in that category
    }
    return item_it->second; // return quantity
}

std::string Inventory::getInventorySummaryJson(const std::string &clientId) const {

    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Search for the client
    auto client_it = m_inventories.find(clientId);
    if (client_it == m_inventories.end()) {
        // if doesn't exists, return an emptty JSON
        return "{}";
    }

    // 2. Convert inventory map into json object with nlohmann library

    json inventory_json = client_it->second;

    // 3. convert to string and ret
    return inventory_json.dump();
}
