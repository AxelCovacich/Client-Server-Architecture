/* Traffic Reporter
# ---------------
# This module is responsible for reporting traffic statistics.
# It provides functionalities to log traffic data and generate reports.
#*/

#ifndef TRAFFIC_REPORTER_HPP
#define TRAFFIC_REPORTER_HPP

#include "prometheus/exposer.h"
#include <atomic>
#include <optional>
#include <prometheus/counter.h>
#include <prometheus/registry.h>
#include <string>
#include <tuple>
#include <vector>

using LabelKey = std::tuple<std::string, std::string>; // protocol, direction
class TrafficReporter {
  public:
    /**
     * @brief Constructs a TrafficReporter object and initializes Prometheus metrics.
     */
    TrafficReporter();

    /**
     * @brief Initializes Prometheus metrics for traffic reporting.
     */
    void initPrometheusMetrics();

    /**
     * @brief Starts the Prometheus HTTP exposer on the specified host and port.
     * @param metricHostPort The host and port in the format "host:port" (e.g., "localhost:8080").
     * @return The exit code of the exposer thread.
     */
    int startPrometheusExposer(const std::string &metricHostPort);

    void incrementMessage(const std::string &protocol, const std::string &direction);
    void incrementError(const std::string &protocol, const std::string &direction);
    void incrementReconnection(const std::string &protocol, const std::string &direction);

    int getMessageCount(const std::string &protocol, const std::string &direction);
    int getErrorCount(const std::string &protocol, const std::string &direction);
    int getReconnectionCount(const std::string &protocol, const std::string &direction);

  private:
    std::shared_ptr<prometheus::Registry> m_registry;
    std::unique_ptr<prometheus::Exposer> m_exposer;
    prometheus::Family<prometheus::Counter> *m_api_msg_counter;
    prometheus::Family<prometheus::Counter> *m_api_error_counter;
    prometheus::Family<prometheus::Counter> *m_api_reconnection_counter;

    /// Map to hold counters for each protocol and direction, and for Label usage (unordered_map not supporting custom
    /// types label)
    std::map<LabelKey, prometheus::Counter *> m_message_counters;
    std::map<LabelKey, prometheus::Counter *> m_error_counters;
    std::map<LabelKey, prometheus::Counter *> m_reconnection_counters;
};

#endif // TRAFFIC_REPORTER_HPP