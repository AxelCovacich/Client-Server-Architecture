#ifndef INVENTORY_HPP
#define INVENTORY_HPP

#include "storage.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <string>

/**
 * @class Inventory
 * @brief Manages the business logic for the application's inventory.
 *
 * This class acts as a facade for all inventory data operations. It encapsulates
 * the underlying storage mechanism (currently an in-memory map) and provides a
 * clean, public API for querying and modifying stock levels, ensuring that all
 * inventory operations are handled consistently.
 */
class Inventory {

  public:
    Inventory(Storage &storage);

    /**
     * @brief Updates the stock quantity for a specific item and client.
     *
     * If the client, category, or item does not already exist, they are
     * created automatically by the underlying map.
     * @param clientId The unique identifier for the client (e.g., "warehouse-A").
     * @param category The item category (e.g., "food", "medicine").
     * @param item The specific item name (e.g., "meat", "bandages").
     * @param quantity The new quantity to set for the item.
     */
    bool updateStock(const std::string &clientId, const std::string &category, const std::string &item, int quantity);

    /**
     * @brief Retrieves the stock quantity of a specific item for a client.
     *
     * This is a read-only method, marked as const. It safely handles cases
     * where the client, category, or item does not exist.
     * @param clientId The unique identifier for the client.
     * @param category The item category.
     * @param item The specific item name.
     * @return The current quantity of the item, or 0 if the item is not found.
     */
    std::optional<int> getStock(const std::string &clientId, const std::string &category, const std::string &item);

    /**
     * @brief Retrieves the stock quantity of a specific item for a client in the cache (map).
     *
     * This method used by getStock to search in local map before going to data base(cache hit)
     * @param clientId The unique identifier for the client.
     * @param category The item category.
     * @param item The specific item name.
     * @return The current quantity of the item saved in ram, or an empty optional if the item is not found in the map.
     */
    std::optional<int> getStockonCache(const std::string &clientId, const std::string &category,
                                       const std::string &item) const;
    /**
     * @brief Gets the entire inventory for a specific client as a JSON string.
     * @param clientId The ID of the client whose inventory is being requested.
     * @return A string containing the inventory in JSON format. Returns an
     * empty JSON object "{}" if the client is not found.
     */
    std::string getInventorySummaryJson(const std::string &clientId);

    /**
     * @brief Gets the entire inventory for a specific client as a JSON string from the cache.
     * @param clientId The ID of the client whose inventory is being requested.
     * @return A string containing the inventory in JSON format. Returns an
     * empty JSON object "{}" if the client is not found.
     */
    json getInventorySummaryJsonFromCache(const std::string &clientId) const;

  private:
    mutable std::mutex m_mutex;
    Storage &m_storage;
    // This map holds the inventory for all clients.
    // Key: clientId, Value: Map of categories for that client.
    std::map<std::string, std::map<std::string, std::map<std::string, int>>> m_inventories;
};

#endif // INVENTORY_HPP