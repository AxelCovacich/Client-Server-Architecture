#ifndef EVENTQUEUE_HPP
#define EVENTQUEUE_HPP

#include "logger.hpp"
#include <deque>
#include <string>
#include <unordered_map>

enum class EventType {
    NOTIFICATION
    // Other event types can be added here
};

struct Event {
    EventType type;
    std::string message;
    // Additional event data can be added here
};

class EventQueue {
  public:
    /**
     * @brief Constructs an EventQueue with a specified maximum size.
     * @param queueSize The maximum number of events to store per client.
     * @param logger A reference to the Logger for logging events and errors.
     */
    EventQueue(int queueSize, Logger &logger);

    /**
     * @brief Pushes a new event onto the queue for a specific client.
     *
     * If the queue exceeds the maximum size, the oldest event is removed.
     * @param clientID The unique identifier of the client.
     * @param event The event to be added to the client's queue.
     */
    void pushEvent(const std::string &clientID, const Event &event);

    /**
     * @brief Pops the oldest event from the queue for a specific client.
     * @param clientID The unique identifier of the client.
     * @param event A reference to an Event object where the popped event will be stored.
     * @return True if an event was successfully popped, false if the queue was empty.
     */
    bool popEvent(const std::string &clientID, Event &event);

  private:
    int m_queueSize;
    std::unordered_map<std::string, std::deque<Event>> m_eventQueueMap;
    Logger &m_logger;
};
#endif // EVENTQUEUE_HPP