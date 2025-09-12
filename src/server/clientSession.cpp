#include "clientSession.hpp"
#include "commandProcessor.hpp"
#include "server.hpp"
#include <array>
#include <chrono>
#include <cstring> // For memset()
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

clientSession::clientSession(int clientSocket, Inventory &inventory, Authenticator &authenticator, Logger &logger,
                             Storage &storage, const std::string &clientIP, SessionManager &sessionManager,
                             const Config &config, TrafficReporter &trafficReporter, EventQueue &eventQueue,
                             UdpHandler &udpHandler)
    : m_clientSocket(clientSocket)
    , m_isAuthenticated(false)
    , m_inventory(inventory)
    , m_authenticator(authenticator)
    , m_logger(logger)
    , m_udpHandler(udpHandler)
    , m_clientIP(clientIP)
    , m_storage(storage)
    , m_sessionManager(sessionManager)
    , m_config(config)
    , m_trafficReporter(trafficReporter)
    , m_eventQueue(eventQueue)
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
                m_trafficReporter.incrementError("tcp", "rx");
                m_logger.log(LogLevel::ERROR, "ClientSession",
                             "Error reading from client socket from IP: " + m_clientIP);
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
            m_trafficReporter.incrementError("tcp", "tx");
            m_logger.log(LogLevel::ERROR, "ClientSession", "Error writing to client socket from IP: " + m_clientIP);
            break;
        }

        if (m_isAuthenticated) {
            handleEventQueue(); // check and send any pending event for this client
        }
        if (!result.second) {
            // close connection
            break;
        }
    }
    // cout << "Closing connection with client from thread: " << this_thread::get_id() << '\n';
    //  if is empty, the client has never autenticathed or registered, nothing to do.
    if (!m_clientID.empty()) {
        m_sessionManager.unregisterSession(m_clientID);
    }

    m_logger.log(LogLevel::INFO, "ClientSession",
                 "Closing connection with client: " + m_clientID + " from IP: " + m_clientIP);
}

bool clientSession::isAuthenticated() const {
    return m_isAuthenticated;
}

clientSession::processResult clientSession::processMessage(const std::string &json_string) {

    // cout << "Thread " << this_thread::get_id() << " received: " << json_string << " from clientIP: " << m_clientIP
    //     << '\n';

    json request = json::parse(json_string, nullptr, false); // don't throw exception, give back discarded if not valid
    if (request.is_discarded()) {

        m_logger.log(LogLevel::WARNING, "ClientSession", "Invalid JSON format from clientIP: " + m_clientIP);
        m_trafficReporter.incrementError("tcp", "rx");
        return {"{\"status\":\"error\",\"message\":\"Invalid JSON format.\"}", true};
    }
    m_trafficReporter.incrementMessage("tcp", "rx");

    // log call with masked password for safety
    m_logger.log(LogLevel::DEBUG, "ClientSession", "Received message: " + createLoggableRequest(request).dump());

    if (!m_isAuthenticated) {
        // Not authenticated, can only log in
        if (request.value("command", "") == "login") { // only log in permitted

            try {

                const std::string user = request.at("payload").at("hostname");
                const std::string pass = request.at("payload").at("password");
                // cout<< "DEBUG: Attempting login for user: " << user << " With password: " << pass << '\n';
                AuthResult result = m_authenticator.authenticate(user, pass, m_config.getBlockTimeSeconds());
                json response;

                switch (result) {
                case AuthResult::SUCCESS:
                    m_isAuthenticated = true;
                    m_clientID = user;
                    // register the active session with clientID + this session object that is running the client. Pass
                    // the shared_ptr of himself
                    m_sessionManager.registerSession(m_clientID, shared_from_this());
                    response["status"] = "success";
                    response["message"] = "Login successful! Welcome " + m_clientID + '.';
                    response["client_id"] = m_clientID;
                    m_logger.log(LogLevel::INFO, "ClientSession",
                                 "Client " + m_clientID + " authenticated successfully from IP: " + m_clientIP);
                    break;
                case AuthResult::FAILED_ACCOUNT_LOCKED:
                    response["status"] = "error";
                    response["message"] = "Account is temporarily locked due to too many failed attempts.";
                    m_trafficReporter.incrementError("tcp", "rx");
                    break;
                case AuthResult::FAILED_ALERT_LOCKED:
                    response["status"] = "error";
                    response["message"] = "Account is locked untill manually freed by an admin, due to alert trigger.";
                    m_trafficReporter.incrementError("tcp", "rx");
                    break;
                case AuthResult::FAILED_USER_NOT_FOUND:
                case AuthResult::FAILED_BAD_CREDENTIALS:
                    response["status"] = "error";
                    response["message"] = "Login failed. Invalid hostname or password.";
                    m_trafficReporter.incrementError("tcp", "rx");
                    break;
                }

                return {response.dump(), true};

            } catch (const json::exception &e) {
                // for any missing item on the json input (payload, hostname or password)

                // std::cerr << "Invalid login request format: " << e.what() << '\n';
                m_logger.log(LogLevel::WARNING, "ClientSession",
                             "Malformed login request from ClientIP: " + m_clientIP);
                json response;
                response["status"] = "error";
                response["message"] =
                    "Malformed login request. Please provide a valid JSON with 'payload', 'hostname' and 'password'.";
                m_trafficReporter.incrementError("tcp", "rx");
                return {response.dump(), true};
            } catch (const std::exception &e) {
                // std::cerr << "CRITICAL ERROR during login attempt from clientIP: " << m_clientIP << ": " << e.what()
                //           << '\n';
                m_logger.log(LogLevel::ERROR, "ClientSession",
                             "CRITICAL: Unhandled exception during login attempt from clientIP: " + m_clientIP +
                                 ". Error: " + e.what());

                json response;
                response["status"] = "error";
                response["message"] = "An internal server error occurred. Please reconnect";
                m_trafficReporter.incrementError("tcp", "rx");
                return {response.dump(), false}; // close connection
            }
        } else {
            // not authenticated, cant process any other command
            json response;
            response["status"] = "error";
            response["message"] = "Authentication required. Please log in first.";
            m_trafficReporter.incrementError("tcp", "rx");
            return {response.dump(), true};
        }

    } else {

        try {

            auto result = commandProcessor::processCommand(request, m_clientID, false, m_inventory, m_logger, m_storage,
                                                           m_sessionManager, m_config, m_trafficReporter);
            return result;

        } catch (const std::exception &e) {
            // std::cerr << "CRITICAL ERROR processing command for client " << m_clientID << ": " << e.what() << '\n';
            m_logger.log(LogLevel::ERROR, "ClientSession",
                         "CRITICAL: Unhandled exception for client " + m_clientID + ". Error: " + e.what());

            json response;
            response["status"] = "error";
            response["message"] = "An internal server error occurred. Please reconnect";
            m_trafficReporter.incrementError("tcp", "rx");
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

std::shared_ptr<sockaddr_storage> clientSession::getUdpAddress() const {

    std::lock_guard<std::mutex> lock(m_sessionMutex);

    return m_udpAddress;
}

void clientSession::setUdpAddress(const struct sockaddr_storage &addr) {

    std::lock_guard<std::mutex> lock(m_sessionMutex);

    m_udpAddress = std::make_shared<sockaddr_storage>(addr);
}

void clientSession::handleEventQueue() {
    Event event;
    while (m_eventQueue.popEvent(m_clientID, event)) {
        switch (event.type) {

        case EventType::NOTIFICATION: {
            if (!m_udpAddress) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    WaitForDataSleepMs)); // give time to the client to send a keepalive and set the udp address
            }
            if (m_udpAddress) {
                m_udpHandler.sendMessageToClient(m_clientID, event.message, *m_udpAddress);
            } else {
                m_logger.log(LogLevel::WARNING, "ClientSession",
                             "UDP address not set for client " + m_clientID + ". Cannot send event.");
            }
        }
            // more event types can be added here
        }
    }
}