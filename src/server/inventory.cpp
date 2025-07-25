#include "inventory.hpp"
#include "storage.hpp"
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

using namespace std;

using json = nlohmann::json;

Inventory::Inventory(Storage &storage)
    : m_storage(storage) {
}

bool Inventory::updateStock(const std::string &clientId, const std::string &category, const std::string &item,
                            int quantity) {

    if (quantity < 0) {
        cout << "Error updating stock.Quantity cannot be negative.\n";
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_inventories[clientId][category][item] = quantity; // CACHE update!

    m_storage.saveStockUpdate(clientId, category, item, quantity);
    // m_log.logStockUpdate(clientID.....)
    return true;
}

std::optional<int> Inventory::getStock(const std::string &clientId, const std::string &category,
                                       const std::string &item) {

    // call for mutex, if its free, just execute, if not wait for mutex to release. Unlocks automaticly by finishing the
    // exec of this function
    std::lock_guard<std::mutex> lock(m_mutex);

    std::optional<int> result = getStockonCache(clientId, category, item);

    if (result.has_value()) {
        // CACHE HIT, no need to go search the data base
        return result.value();
    }
    // Cache miss, must go to data base
    result = m_storage.getStock(clientId, category, item);

    if (result.has_value()) {
        // update stock in cache.
        m_inventories[clientId][category][item] = result.value();
        return result.value();
    }
    // no result found anywhere
    return std::nullopt;
}

std::optional<int> Inventory::getStockonCache(const std::string &clientId, const std::string &category,
                                              const std::string &item) const {

    auto client_it = m_inventories.find(clientId);
    if (client_it == m_inventories.end()) {
        return std::nullopt; // could't find inventory for client
    }

    auto category_it = client_it->second.find(category);
    if (category_it == client_it->second.end()) {
        return std::nullopt; // category doesn't exists
    }

    auto item_it = category_it->second.find(item);
    if (item_it == category_it->second.end()) {
        return std::nullopt; // no item in that category
    }
    return item_it->second; // return quantity
}

std::string Inventory::getInventorySummaryJson(const std::string &clientId) {

    std::lock_guard<std::mutex> lock(m_mutex);

    json inventoryData = getInventorySummaryJsonFromCache(clientId);

    if (!inventoryData.empty()) {
        // Cache hit!
        return inventoryData.dump();
    }

    std::optional<json> result = m_storage.getFullInventory(clientId);
    if (!result.has_value()) {
        // no client or client has no values charged
        return "{}";
    }
    inventoryData = *result; // extract the json from the option variable

    // update the cache!
    m_inventories[clientId] = inventoryData.get<std::map<std::string, std::map<std::string, int>>>();
    // 3. convert to string and ret
    return inventoryData.dump();
}

json Inventory::getInventorySummaryJsonFromCache(const std::string &clientId) const {

    // 1. Search for the client
    auto client_it = m_inventories.find(clientId);
    if (client_it == m_inventories.end()) {
        // if doesn't exists, return an empty JSON
        return json::object();
    }

    return client_it->second;
}