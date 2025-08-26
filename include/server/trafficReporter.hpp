/* Traffic Reporter
# ---------------
# This module is responsible for reporting traffic statistics.
# It provides functionalities to log traffic data and generate reports.
#*/

#ifndef TRAFFIC_REPORTER_HPP
#define TRAFFIC_REPORTER_HPP

#include <atomic>
#include <optional>
#include <string>
#include <vector>

class TrafficReporter {
  public:
    TrafficReporter() = default;

    void onMessageReceived() {
        m_stats.messageCount++;
    }
    void onError() {
        m_stats.errorCount++;
    }
    void onReconnection() {
        m_stats.reconnectionCount++;
    }

    std::string generateReport() const;

  private:
    // Store traffic data
    struct TrafficData {
        std::atomic<int> messageCount{0};
        std::atomic<int> errorCount{0};
        std::atomic<int> reconnectionCount{0};
    };

    TrafficData m_stats;
};

#endif // TRAFFIC_REPORTER_HPP