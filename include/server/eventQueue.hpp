#ifndef EVENTQUEUE_HPP
#define EVENTQUEUE_HPP

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
    EventQueue(int queueSize);
    void pushEvent(const std::string &clientID, const Event &event);
    bool popEvent(const std::string &clientID, Event &event);

  private:
    int m_queueSize;
    std::unordered_map<std::string, std::deque<Event>> m_eventQueueMap;
};
#endif // EVENTQUEUE_HPP