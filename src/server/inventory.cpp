#include "inventory.hpp"
#include "storage.hpp"
#include <iostream>
#include <map>
#include <optional>
#include <string>

using namespace std;

Inventory::Inventory(Storage &storage, Logger &logger)
    : m_storage(storage)
    , m_logger(logger) {
}

bool Inventory::updateStock(const std::string &clientId, const std::string &category, const std::string &item,
                            int quantity) {

    if (quantity < 0) { // this check is for the cache, db table has his own check
        m_logger.log(LogLevel::WARNING, "Inventory",
                     "Client " + clientId + " trying to update negative quantity for item '" + item + "'.");

        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_inventories[clientId][category][item] = quantity; // CACHE update!

    m_storage.saveStockUpdate(clientId, category, item, quantity);
    m_logger.log(LogLevel::INFO, "Inventory",
                 "Stock succesfully updated for " + category + ":" + item + " to " + std::to_string(quantity) +
                     " for client " + clientId,
                 clientId);
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
        // m_logger.log(LogLevel::DEBUG, "Inventory", "Stock query for item '" + item + "'.", clientId);
        return result.value();
    }
    // Cache miss, must go to data base
    result = m_storage.getStock(clientId, category, item);

    if (result.has_value()) {
        // update stock in cache.
        m_inventories[clientId][category][item] = result.value();
        // m_logger.log(LogLevel::DEBUG, "Inventory", "Stock query for item '" + item + "'.", clientId);
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

std::optional<Inventory::ClientInventoryMap> Inventory::getInventorySummary(const std::string &clientId) {

    std::lock_guard<std::mutex> lock(m_mutex);

    auto inventoryData = getInventorySummaryFromCache(clientId);
    m_logger.log(LogLevel::INFO, "Inventory", "Full inventory summary requested for client " + clientId + ".",
                 clientId);

    if (inventoryData.has_value()) {
        // Cache hit!
        return inventoryData;
    }

    auto result = m_storage.getFullInventory(clientId);
    if (!result.has_value()) {
        // no client or client has no values charged
        return std::nullopt;
    }

    // update the cache!
    m_inventories[clientId] = *result;
    return result;
}

std::optional<Inventory::ClientInventoryMap>
Inventory::getInventorySummaryFromCache(const std::string &clientId) const {

    // 1. Search for the client
    auto client_it = m_inventories.find(clientId);
    if (client_it == m_inventories.end()) {
        // doesn't exist
        return std::nullopt;
    }

    return client_it->second;
}