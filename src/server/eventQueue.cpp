#include "eventQueue.hpp"
#include <iostream>

using namespace std;

EventQueue::EventQueue(int queueSize, Logger &logger)
    : m_eventQueueMap()
    , m_queueSize(queueSize)
    , m_logger(logger) {
    // Constructor implementation (if needed)
}

void EventQueue::pushEvent(const std::string &clientID, const Event &event) {
    // no need to initialize the deque, operator[] does it if not exists
    if (m_eventQueueMap[clientID].size() >= m_queueSize) {
        m_eventQueueMap[clientID].pop_front(); // Remove oldest event if at capacity
        m_logger.log(LogLevel::WARNING, "EventQueue",
                     "Event queue for client " + clientID + " is full. Oldest event dropped.");
    }
    m_eventQueueMap[clientID].push_back(event);
    m_logger.log(LogLevel::INFO, "EventQueue", "Event pushed to queue for client " + clientID);
}

bool EventQueue::popEvent(const std::string &clientID, Event &event) {
    auto tempIt = m_eventQueueMap.find(clientID);

    if (tempIt != m_eventQueueMap.end() && !tempIt->second.empty()) {
        event = tempIt->second.front();
        tempIt->second.pop_front();
        // Optionally remove the deque if it's empty to save space
        if (tempIt->second.empty()) {
            m_eventQueueMap.erase(tempIt);
        }
        m_logger.log(LogLevel::INFO, "EventQueue", "Event popped from queue for client " + clientID);
        return true;
    }
    m_logger.log(LogLevel::INFO, "EventQueue", "No events to pop for client " + clientID);
    return false; // No events for this clientID
}