#include "clientSession.hpp"
#include "authenticator.hpp"
#include "commandProcessor.hpp"
#include "server.hpp"
#include <cstring> // For memset()
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

clientSession::clientSession(int clientSocket, Inventory &inventory, Authenticator &authenticator)
    : m_clientSocket(clientSocket)
    , m_clientID("")
    , // starts empty
    m_isAuthenticated(false)
    , m_inventory(inventory)
    , m_authenticator(authenticator) {
    // constructor actions here
}

clientSession::~clientSession() {
    if (m_clientSocket != -1) {
        cout << "Closing client socket.\n";
        close(m_clientSocket);
    }
}

void clientSession::run() {
    cout << "New client connected. Handled by thread: " << this_thread::get_id() << '\n';

    const char *welcome_msg =
        "Welcome to the C++ Server! Please login to initiate operations or type 'end' to disconnect.\n";
    const char *ack_msg = "ACK: Message received by server.\n";

    if (write(m_clientSocket, welcome_msg, strlen(welcome_msg)) < 0) {
        perror("Writing to client socket");
        close(m_clientSocket);
        return;
    }

    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];

    while (true) {

        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = read(m_clientSocket, buffer, BUFFER_SIZE - 1);

        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                perror("Error reading from socket");
            }
            cout << "Client disconnected. Thread " << this_thread::get_id() << " finishing.\n";
            break;
        }

        buffer[bytes_read] = '\0';

        string client_message(buffer);

        processResult result = processMessage(client_message);

        if (write(m_clientSocket, result.first.c_str(), result.first.length()) < 0) {

            perror("Error writing response to socket");
            break;
        }

        if (!result.second) {

            cout << "Closing connection based on command.\n";
            break;
        }
    }
    cout << "Closing connection with client from thread: " << this_thread::get_id() << '\n';
    close(m_clientSocket);
}

bool clientSession::isAuthenticated() const {
    return m_isAuthenticated;
}

clientSession::processResult clientSession::processMessage(const std::string &json_string) {

    cout << "Thread " << this_thread::get_id() << " received: " << json_string << "from client " << m_clientID << '\n';

    json request = json::parse(json_string, nullptr, false); // don't throw exception, give back discarded if not valid
    if (request.is_discarded()) {

        return {"{\"status\":\"error\",\"message\":\"Invalid JSON format.\"}", true};
    }

    if (!m_isAuthenticated) {
        // Not authenticated, can only log in
        if (request.value("command", "") == "login") { // only log in permitted

            try {

                const std::string user = request.at("payload").at("hostname");
                const std::string pass = request.at("payload").at("password");

                if (m_authenticator.authenticate(user, pass)) {
                    m_isAuthenticated = true;
                    m_clientID = user;
                    return {"{\"status\":\"success\",\"message\":\"Login successful.\"}", true};
                } else {
                    return {"{\"status\":\"error\",\"message\":\"Login failed. Invalid credentials.\"}", true};
                }
            } catch (const json::exception &e) {
                // for any missing item on the json input (payload, hostname or password)
                std::cerr << "Invalid login request format: " << e.what() << '\n';
                return {"{\"status\":\"error\",\"message\":\"Malformed login request.\"}", true};
            }
        } else {
            // not authenticated, cant process any other command
            return {"{\"status\":\"error\",\"message\":\"Authentication required.\"}", true};
        }

    } else {
        // succesfully logged in here. Can do operations.
        cout << "Thread " << this_thread::get_id() << " received: " << json_string << "from client " << m_clientID
             << '\n';

        return commandProcessor::processCommand(request, m_clientID, false, m_inventory);
    }
}