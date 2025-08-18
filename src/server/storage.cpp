#include "storage.hpp"
#include "bcrypt.h"
#include <SQLiteCpp/Statement.h>
#include <ctime>
#include <iostream>
#include <optional>

using namespace std;

Storage::Storage(const std::string &dbPath, int openFlags)
    : // constructor call of the db object, path to file and flags given. Read and write by default
    m_db(dbPath, openFlags) {
}

Storage::~Storage() {
    // Nothing to do here, SQLite::Database destructor is auto
}

void Storage::initializeSchema() {

    try {
        cout << "Initializing database schema...\n";

        m_db.exec(
            "PRAGMA foreign_keys = ON;"); // CLIENT_ID IS A FOREIGN KEY NOW. CLIENT_ID MUST BE A VALID USER-TABLE MEMBER

        m_db.exec(R"(
            CREATE TABLE IF NOT EXISTS users (
                hostname      TEXT PRIMARY KEY NOT NULL    CHECK (length(hostname) > 0),
                password_hash TEXT NOT NULL,
                failed_attempts INTEGER DEFAULT 0          CHECK(failed_attempts <= 3),
                last_failed_timestamp INTEGER DEFAULT 0,
                is_locked     INTEGER DEFAULT 0
            );
        )");

        cout << "Users table initialized successfully.\n";

        // "string literal raw"  C++ (R"()") for commas use
        // compound primary key:unique combinations of clientid,category and item
        m_db.exec(R"(
            CREATE TABLE IF NOT EXISTS inventory (
                client_id TEXT NOT NULL     CHECK (length(client_id) > 0),
                category  TEXT NOT NULL     CHECK (length(category) > 0),
                item      TEXT NOT NULL     CHECK (length(item) > 0),
                quantity  INTEGER NOT NULL,
                PRIMARY KEY (client_id, category, item),
                FOREIGN KEY (client_id) REFERENCES users(hostname)
            );
        )");

        cout << "Inventory table initialized successfully.\n";

        m_db.exec(R"(
            CREATE TABLE IF NOT EXISTS logs (
                log_id    INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp TEXT NOT NULL,
                level     TEXT NOT NULL,
                component TEXT NOT NULL,
                message   TEXT NOT NULL,
                client_id TEXT,
                FOREIGN KEY (client_id) REFERENCES users(hostname)
            );
        )");
        cout << "Logs table initialized successfully.\n";

    } catch (const SQLite::Exception &e) {
        std::cerr << "Error initializing schema: " << e.what() << '\n';
        // so main program who call knows about failure
        throw;
    }
}

void Storage::saveStockUpdate(const std::string &clientId, const std::string &category, const std::string &item,
                              int quantity) {
    try {
        // prepare statement for sql library this way to avoid sqlinjection. insert or update,
        // if already exists(on conflict), just update quantity value
        SQLite::Statement query(m_db, R"(
                INSERT INTO inventory (client_id, category, item, quantity)
                VALUES (?, ?, ?, ?)
                ON CONFLICT(client_id, category, item) DO UPDATE SET
                quantity = excluded.quantity;
            )");

        // Assign the values to ? Order is important, first ? is clientId
        query.bind(1, clientId);
        query.bind(2, category);
        query.bind(3, item);
        query.bind(4, quantity);

        // execute the order to the db
        query.exec();

    } catch (const std::exception &e) {
        std::cerr << "Error saving stock update to database: " << e.what() << '\n';
        throw;
    }
}

std::optional<int> Storage::getStock(const std::string &clientId, const std::string &category,
                                     const std::string &item) {

    try {
        SQLite::Statement query(m_db,
                                "SELECT quantity FROM inventory WHERE client_id = ? AND category = ? AND item = ?");
        query.bind(1, clientId);
        query.bind(2, category);
        query.bind(3, item);

        if (query.executeStep()) {
            return query.getColumn("quantity").getInt();
        }

        std::cerr << "No record found for the specified item.\n";
        return std::nullopt;

    } catch (const std::exception &e) {
        std::cerr << "Error at getting stock from database " << e.what() << '\n';
        throw;
    }
}

std::optional<Storage::ClientInventoryMap> Storage::getFullInventory(const std::string &clientId) {

    try {
        SQLite::Statement query(m_db, "SELECT category, item, quantity FROM inventory WHERE client_id = ?");
        query.bind(1, clientId);

        ClientInventoryMap inventoryMap;
        bool client_found = false;

        // Must iterate over all the rows found for client
        while (query.executeStep()) {
            client_found = true;
            // now extract each column for the row found
            std::string category = query.getColumn("category");
            std::string item = query.getColumn("item");
            int quantity = query.getColumn("quantity");

            // save the data row by row
            inventoryMap[category][item] = quantity;
        }
        if (!client_found) {
            // Client doesn't exist or has empty values
            return std::nullopt;
        }

        return inventoryMap;

    } catch (const std::exception &e) {
        std::cerr << "Error getting full inventory from database: " << e.what() << '\n';
        throw;
    }
}

void Storage::createUser(const std::string &hostname, const std::string &password) {
    try {

        std::string password_hash = bcrypt::generateHash(password);

        SQLite::Statement query(m_db, R"(
            INSERT INTO users (hostname, password_hash) 
            VALUES (?, ?); 
        )");

        query.bind(1, hostname);
        query.bind(2, password_hash);

        query.exec();

        std::cout << "User '" << hostname << "' created or already exists.\n";

    } catch (const std::exception &e) {
        std::cerr << "Error creating user '" << hostname << "': " << e.what() << '\n';
        throw;
    }
}

bool Storage::userExists(const std::string &hostname) {
    try {
        SQLite::Statement query(m_db, "SELECT COUNT(*) FROM users WHERE hostname = ?");
        query.bind(1, hostname);

        query.executeStep();
        return (query.getColumn(0).getInt() == 1); // if user not found, count will be 0

    } catch (const std::exception &e) {
        std::cerr << "Error checking if user exists: " << e.what() << '\n';
        throw;
    }
}

std::optional<userAuthData> Storage::getUserLoginData(const std::string &hostname) {

    try {
        SQLite::Statement query(
            m_db,
            "SELECT password_hash, failed_attempts, last_failed_timestamp, is_locked FROM users WHERE hostname = ?");
        query.bind(1, hostname);

        if (query.executeStep()) {
            userAuthData userData;
            userData.passwordHash = query.getColumn("password_hash").getString();
            userData.failedAttempts = query.getColumn("failed_attempts").getInt();
            userData.lastFailedTimestamp = query.getColumn("last_failed_timestamp").getInt();
            userData.is_locked = (query.getColumn("is_locked").getInt() == 1);
            return userData;
        }

        return std::nullopt; // no user registered for the hostname given
    } catch (const std::exception &e) {
        std::cerr << "Error getting user data login from data base: " << e.what() << '\n';
        throw;
    }
}

void Storage::updateLoginAttempts(const std::string &hostname, bool loginSuccess, time_t timeStamp) {

    try {

        if (loginSuccess) {

            SQLite::Statement query(m_db, "UPDATE users SET failed_attempts = 0 WHERE hostname = ?");
            query.bind(1, hostname);
            query.exec();
        } else {
            SQLite::Statement query(
                m_db,
                "UPDATE users SET failed_attempts = failed_attempts + 1, last_failed_timestamp = ? WHERE hostname = ?");
            query.bind(1, static_cast<int64_t>(timeStamp));
            query.bind(2, hostname);
            query.exec();
        }
    } catch (const std::exception &e) {
        std::cerr << "Error updating login attempts for user '" << hostname << "': " << e.what() << '\n';
        throw;
    }
}

void Storage::saveLogEntry(const std::string &date, const std::string &level, const std::string &component,
                           const std::string &message, const std::optional<std::string> &clientId) {
    try {

        SQLite::Statement query(m_db, R"(
                                            INSERT INTO logs (timestamp, level, component, message,client_id)
                                            VALUES (?, ?, ?, ?, ?);)");
        query.bind(1, date);
        query.bind(2, level);
        query.bind(3, component);
        query.bind(4, message);
        if (clientId.has_value()) {
            query.bind(5, clientId.value()); // NOLINT
        }
        query.exec();
    } catch (const std::exception &e) {
        throw; // logger function will attend the excep
    }
}

std::vector<LogEntry> Storage::getInventoryHistoryTransaction(const std::string &clientId) {
    std::vector<LogEntry> history;
    try {
        SQLite::Statement query(m_db, R"(
                                        SELECT timestamp, level, component, message 
                                        FROM logs
                                        WHERE client_id = ? AND component = 'Inventory'
                                        ORDER BY timestamp DESC
                                        )");

        query.bind(1, clientId);
        bool client_found = false;

        // Must iterate over all the rows found for client, pushback will add the logentry object with all elements at
        // the end of the vector history.
        while (query.executeStep()) {
            history.push_back({.timestamp = query.getColumn("timestamp").getString(),
                               .level = query.getColumn("level").getString(),
                               .component = query.getColumn("component").getString(),
                               .message = query.getColumn("message").getString()});
        }
    } catch (const std::exception &e) {
        std::cerr << "Error getting full history for inventory from database: " << e.what() << '\n';
        throw;
    }
    return history;
}

bool Storage::setClientLockStatus(const std::string &hostname, bool isLocked) {
    try {
        SQLite::Statement query(m_db, "UPDATE users SET is_locked = ? WHERE hostname = ?");
        query.bind(
            1, isLocked ? 1 : 0); // based on bool isLocked, if got to lock, will write 1, if have to unlock, write a 0
        query.bind(2, hostname);
        int rowsModified = query.exec();
        std::cout << "DEBUG: UPDATE for user '" << hostname << "' modified " << rowsModified << " rows.\n";
        return (rowsModified == 1); // if no rows were modified, the client doesn't exists, return false

    } catch (const std::exception &e) {
        std::cerr << "Error trying to set the lock status of client in data base: " << e.what() << '\n';
        throw;
    }
}

bool Storage::isClientLocked(const std::string &hostname) {
    try {
        SQLite::Statement query(m_db, "SELECT is_locked FROM users WHERE hostname = ?");
        query.bind(1, hostname);
        if (query.executeStep()) {
            return (query.getColumn(0).getInt() == 1);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error trying to get status of client in data base: " << e.what() << '\n';
        throw;
    }
    return false; // if didn't find the user just return false
}