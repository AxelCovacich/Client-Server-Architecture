#include "clientSession.hpp"
#include "authenticator.hpp"
#include "commandProcessor.hpp"
#include "server.hpp"
#include <array>
#include <cstring> // For memset()
#include <iostream>

#include <thread>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

clientSession::clientSession(int clientSocket, Inventory &inventory, Authenticator &authenticator, Logger &logger,
                             Storage &storage, const std::string &clientIP)
    : m_clientSocket(clientSocket)
    , m_isAuthenticated(false)
    , m_inventory(inventory)
    , m_authenticator(authenticator)
    , m_logger(logger)
    , m_clientIP(clientIP)
    , m_storage(storage)
// m_clientID("") starts empty already
{
    // constructor actions here
}

clientSession::~clientSession() {
    if (m_clientSocket != -1) {
        cout << "Closing client socket.\n";
        close(m_clientSocket);
    }
}

void clientSession::run() {

    m_logger.log(LogLevel::INFO, "ClientSession", "New connection accepted from IP " + m_clientIP);

    const char *welcome_msg =
        "Welcome to the C++ Server! Please login to start operating or type 'end' to disconnect.\n";

    if (write(m_clientSocket, welcome_msg, strlen(welcome_msg)) < 0) {
        perror("Writing to client socket");
        close(m_clientSocket);
        return;
    }

    ssize_t bytes_read = -1;
    std::array<char, BUFFER_SIZE> buffer{};

    while (true) {

        buffer.fill('\0'); // fill the buffer with 0 Just like memset
        bytes_read = read(m_clientSocket, buffer.data(), buffer.size() - 1);

        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                perror("Error reading from socket");
            }
            m_logger.log(LogLevel::WARNING, "ClientSession",
                         "Client connection from IP: " + m_clientIP + "has been disconnected.");
            // cout << "Client disconnected. Thread " << this_thread::get_id() << " finishing.\n";
            break;
        }

        buffer.at(bytes_read) = '\0'; // to make it "c string friendly"

        string client_message(buffer.data());

        processResult result = processMessage(client_message);

        if (write(m_clientSocket, result.first.c_str(), result.first.length()) < 0) {

            perror("Error writing response to socket");
            break;
        }

        if (!result.second) {
            // close connection
            break;
        }
    }
    // cout << "Closing connection with client from thread: " << this_thread::get_id() << '\n';
    m_logger.log(LogLevel::INFO, "ClientSession",
                 "Closing connection with client: " + m_clientID + " from IP: " + m_clientIP);
    close(m_clientSocket);
}

bool clientSession::isAuthenticated() const {
    return m_isAuthenticated;
}

clientSession::processResult clientSession::processMessage(const std::string &json_string) {

    // cout << "Thread " << this_thread::get_id() << " received: " << json_string << "from client " << m_clientID <<
    // '\n';

    json request = json::parse(json_string, nullptr, false); // don't throw exception, give back discarded if not valid
    if (request.is_discarded()) {

        m_logger.log(LogLevel::WARNING, "ClientSession", "Invalid JSON format from clientIP: " + m_clientIP);

        return {"{\"status\":\"error\",\"message\":\"Invalid JSON format.\"}", true};
    }

    // log call with masked password for safety
    m_logger.log(LogLevel::DEBUG, "ClientSession", "Received message: " + createLoggableRequest(request).dump());

    if (!m_isAuthenticated) {
        // Not authenticated, can only log in
        if (request.value("command", "") == "login") { // only log in permitted

            try {

                const std::string user = request.at("payload").at("hostname");
                const std::string pass = request.at("payload").at("password");

                AuthResult result = m_authenticator.authenticate(user, pass);
                json response;

                switch (result) {
                case AuthResult::SUCCESS:
                    m_isAuthenticated = true;
                    m_clientID = user;
                    response["status"] = "success";
                    response["message"] = "Login successful.";
                    break;
                case AuthResult::FAILED_ACCOUNT_LOCKED:
                    response["status"] = "error";
                    response["message"] = "Account is temporarily locked due to too many failed attempts.";
                    break;
                case AuthResult::FAILED_USER_NOT_FOUND:
                case AuthResult::FAILED_BAD_CREDENTIALS:
                    response["status"] = "error";
                    response["message"] = "Login failed. Invalid hostname or password.";
                    break;
                }

                return {response.dump(), true};

            } catch (const json::exception &e) {
                // for any missing item on the json input (payload, hostname or password)

                // std::cerr << "Invalid login request format: " << e.what() << '\n';
                m_logger.log(LogLevel::WARNING, "ClientSession",
                             "Malformed login request from ClientIP: " + m_clientIP);
                return {"{\"status\":\"error\",\"message\":\"Malformed login request.\"}", true};
            }
        } else {
            // not authenticated, cant process any other command
            return {"{\"status\":\"error\",\"message\":\"Authentication required.\"}", true};
        }

    } else {

        try {

            auto result =
                commandProcessor::processCommand(request, m_clientID, false, m_inventory, m_logger, m_storage);
            return result;

        } catch (const std::exception &e) {
            // std::cerr << "CRITICAL ERROR processing command for client " << m_clientID << ": " << e.what() << '\n';
            m_logger.log(LogLevel::ERROR, "ClientSession",
                         "CRITICAL: Unhandled exception for client " + m_clientID + ". Error: " + e.what());

            json response;
            response["status"] = "error";
            response["message"] = "An internal server error occurred. Please reconnect";

            return {response.dump(), false};
        }
    }
}

json clientSession::createLoggableRequest(json request) {
    if (request.contains("payload")) {
        if (request["payload"].contains("password")) {
            request["payload"]["password"] = "[REDACTED]";
        }
    }
    return request;
}