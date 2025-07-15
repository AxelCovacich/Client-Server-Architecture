#include "commandProcessor.hpp"

namespace commandProcessor {

commandResult processCommand(const std::string &command) {
    if (command == "status") {
        return {"STATUS: OK\n", true};
    }
    if (command == "end") {
        return {"ACK: Disconnect command received.\n", false};
    }
    return {"ACK: Message received.\n", true};
}

} // namespace commandProcessor