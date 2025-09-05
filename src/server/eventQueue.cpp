#include "eventQueue.hpp"
#include <iostream>

EventQueue::EventQueue(int queueSize)
    : m_eventQueueMap()
    , m_queueSize(queueSize) {
    // Constructor implementation (if needed)
}

void EventQueue::pushEvent(const std::string &clientID, const Event &event) {
    // no need to initialize the deque, operator[] does it if not exists
    if (m_eventQueueMap[clientID].size() >= m_queueSize) {
        m_eventQueueMap[clientID].pop_front(); // Remove oldest event if at capacity
    }
    m_eventQueueMap[clientID].push_back(event);
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
        return true;
    }
    return false; // No events for this clientID
}