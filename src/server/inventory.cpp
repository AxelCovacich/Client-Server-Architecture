#include "inventory.hpp"
#include "storage.hpp"
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>

using namespace std;

Inventory::Inventory(Storage &storage, Logger &logger)
    : m_storage(storage)
    , m_logger(logger) {
}

UpdateStockResult Inventory::updateStock(const std::string &clientId, const std::string &category,
                                         const std::string &item, int quantity) {

    if (quantity < 0) { // this check is for the cache, db table has his own check
        m_logger.log(LogLevel::WARNING, "Inventory",
                     "Client " + clientId + " trying to update negative quantity for item '" + item + "'.");

        return {false, "Update rejected: Quantity cannot be negative."};
    }

    auto result = validateInventory(category, item);
    if (!result.success) {
        return result;
    }

    m_storage.saveStockUpdate(clientId, category, item, quantity);
    m_logger.log(LogLevel::INFO, "Inventory",
                 "Stock succesfully updated for " + category + ":" + item + " to " + std::to_string(quantity) +
                     " for client " + clientId,
                 clientId);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_inventories[clientId][category][item] = quantity; // CACHE update!
    return {true,
            "Stock for '" + category + ":" + item + "' successfully updated to " + std::to_string(quantity) + "."};
}

std::optional<int> Inventory::getStock(const std::string &clientId, const std::string &category,
                                       const std::string &item) {

    std::optional<int> result = getStockonCache(clientId, category, item);

    if (result.has_value()) {
        // CACHE HIT, no need to go search the data base
        // m_logger.log(LogLevel::DEBUG, "Inventory", "Stock query for item '" + item + "'.", clientId);
        return result.value();
    }
    // Cache miss, must go to data base
    result = m_storage.getStock(clientId, category, item);

    if (result.has_value()) {
        // update stock in cache.
        std::lock_guard<std::mutex> lock(m_mutex);
        m_inventories[clientId][category][item] = result.value();
        // m_logger.log(LogLevel::DEBUG, "Inventory", "Stock query for item '" + item + "'.", clientId);
        return result.value();
    }
    // no result found anywhere
    return std::nullopt;
}

std::optional<int> Inventory::getStockonCache(const std::string &clientId, const std::string &category,
                                              const std::string &item) const {
    std::lock_guard<std::mutex> lock(m_mutex);
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

std::optional<Inventory::ClientInventoryMap> Inventory::getInventorySummary(const std::string &clientId) {

    m_logger.log(LogLevel::INFO, "Inventory", "Full inventory summary requested for client " + clientId + ".",
                 clientId);

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_fullyCachedClients.contains(clientId)) {
        // Cache Hit!
        return getInventorySummaryFromCache(clientId);
    }

    auto result = m_storage.getFullInventory(clientId);
    if (!result.has_value()) {
        // no client or client has no values charged
        return std::nullopt;
    }

    // update the cache!
    m_inventories[clientId] = *result;
    m_fullyCachedClients.insert(clientId);
    return result;
}

std::optional<Inventory::ClientInventoryMap>
Inventory::getInventorySummaryFromCache(const std::string &clientId) const {

    // Search for the client
    auto client_it = m_inventories.find(clientId);
    if (client_it == m_inventories.end()) {
        // doesn't exist
        return std::nullopt;
    }

    return client_it->second;
}

// map of valid inventory categories with a set of valid items each.
const std::map<std::string, std::set<std::string>> Inventory::s_validItems = {
    {"food", {"meat", "vegetables", "fruits", "water", "bread"}},
    {"medicine", {"antibiotics", "analgesics", "bandages"}},
    {"ammo", {"pistol_rounds", "shotgun_shells", "grenades"}}};

UpdateStockResult Inventory::validateInventory(const std::string &category, const std::string &item) {

    auto categoryPtr = s_validItems.find(category);

    if (categoryPtr == s_validItems.end()) {
        // category not found
        // m_logger.log(LogLevel::WARNING, "Inventory", "Update rejected: Invalid category '" + category + "'",
        // clientId);
        return {false, "Update rejected: Invalid category '" + category + "'."};
    }

    if (categoryPtr->second.find(item) == categoryPtr->second.end()) {
        // item not found
        // m_logger.log(LogLevel::WARNING, "Inventory", "Update rejected: Invalid category '" + category + "'",
        // clientId);
        return {false, "Update rejected: Invalid item '" + item + "'."};
    }
    return {true, "valid ok"};
}
