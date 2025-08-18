#ifndef IPC_HANDLER_HPP
#define IPC_HANDLER_HPP

#include "alertManager.hpp"
#include "logger.hpp"
#include <array>
#include <string>

#define BUFFER_SIZE 256
/**
 * @class IpcHandler
 * @brief Handles incoming IPC (Inter-Process Communication) connections.
 *
 * This class encapsulates the logic for accepting connections on the Unix domain
 * socket and passing the received alert data to the AlertManager.
 */
class IpcHandler {
  public:
    /**
     * @brief Constructs a new IpcHandler.
     * @param ipcSocketFd The server's listening IPC socket file descriptor.
     * @param logger A reference to the shared server logger.
     * @param alertManager A reference to the alert management module.
     */
    IpcHandler(Logger &logger, AlertManager &alertManager);

    /**
     * @brief The main entry point for handling an IPC connection.
     *
     * Called by the Server's main loop when IPC activity is detected. It accepts
     * the connection, reads the alert message, and forwards it to the AlertManager.
     */
    bool handleConnection(int acceptedSocketFd);

  private:
    Logger &m_logger;
    AlertManager &m_alertManager;
};

#endif // IPC_HANDLER_HPP