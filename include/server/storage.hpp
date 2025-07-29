#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <SQLiteCpp/Database.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct userAuthData {
    std::string passwordHash;
    int failedAttempts;
    time_t lastFailedTimestamp;
};

struct LogEntry {
    std::time_t timestamp;
    std::string level;
    std::string component;
    std::string message;
    std::optional<std::string> clientId;
};

/**
 * @class Storage
 * @brief Manages all persistence operations using a SQLite database.
 *
 * This class is the single point of contact with the database file. It handles
 * the connection, schema initialization, and all read/write queries.
 */
class Storage {
  public:
    using ClientInventoryMap = std::map<std::string, std::map<std::string, int>>;

    /**
     * @brief Constructs a Storage object and opens the database connection on read-write by default.
     * @param dbPath The file path to the SQLite database.
     * @throw SQLite::Exception if the database cannot be opened.
     */
    Storage(const std::string &dbPath, int openFlags = SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    /**
     * @brief The destructor automatically closes the database connection.
     */
    ~Storage();

    SQLite::Database &getDb() {
        return m_db;
    } // getter to the db object(reference)

    /**
     * @brief Initializes the database schema if it doesn't exist. It creates inventory and users tables.
     */
    void initializeSchema();

    /**
     * @brief Inserts or updates a single item's stock in the database.
     *
     * Performs an "UPSERT" operation. If the item record already exists for the
     * client/category, its quantity is updated. Otherwise, a new record is inserted.
     * @param clientId The client's unique identifier.
     * @param category The item's category (e.g., "food").
     * @param item The item's name (e.g., "meat").
     * @param quantity The new quantity to set.
     * @throw SQLite::Exception if an error ocurres trying to write to db(eg:table doesn't exists)
     */
    void saveStockUpdate(const std::string &clientId, const std::string &category, const std::string &item,
                         int quantity);

    /**
     * @brief Retrieves the quantity of a single item from the database.
     * @param clientId The client's unique identifier.
     * @param category The item's category.
     * @param item The item's name.
     * @return An optional containing the quantity if the item is found, otherwise an empty optional.
     * @throw SQLite::Exception if an error ocurrs while reading.
     */
    std::optional<int> getStock(const std::string &clientId, const std::string &category, const std::string &item);

    /**
     * @brief Retrieves the entire inventory for a specific client.
     * @param clientId The client's unique identifier.
     * @return An optional containing a map object of the client's full inventory
     * if found, otherwise an empty optional.
     * @throw SQLite::Exception if an error ocurrs trying to read.
     */
    std::optional<ClientInventoryMap> getFullInventory(const std::string &clientId);

    /**
     * @brief Creates a new user record with a hashed password in the database.
     * @param `hostname` The user given hostname.
     * @param `password` The user given password.
     */
    void createUser(const std::string &hostname, const std::string &password);

    /**
     * @brief Checks if a user with the specified hostname exists in the database.
     * @return `TRUE` if the user exists `FALSE` otherwise
     */
    bool userExists(const std::string &hostname);

    /**
     * @brief Retrieves the authentication data (e.g., hash, failed attempts) for a given user.
     * @param hostname The user hostname.
     * @return An optional struct userAuthData with the passwordHash and failed attempts or a nullopt if didn't find any
     * data
     */
    std::optional<userAuthData> getUserLoginData(const std::string &hostname);

    /**
     * @brief Updates the failed login attempt counter for a user.Resets the counter to 0 on a successful login or
     * increments it on a failure.
     * @param hostname The user's hostname.
     * @param loginSuccess True if the login was successful, false otherwise.
     * @param timeStamp Time of the last failed attempt.
     */
    void updateLoginAttempts(const std::string &hostname, bool loginSuccess, time_t timeStamp);

    void saveLogEntry(std::time_t timestamp, const std::string &level, const std::string &component,
                      const std::string &message, const std::optional<std::string> &clientId = std::nullopt);

    std::vector<LogEntry> getInventoryHistoryTransaction(const std::string &clientId);

  private:
    // Library SQLiteCpp wrappes the connection to the data base with SQLite::Database. We use the object of the
    // library.
    SQLite::Database m_db;
};

#endif // STORAGE_HPP