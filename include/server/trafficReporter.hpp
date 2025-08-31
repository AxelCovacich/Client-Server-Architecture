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
    TrafficReporter();
    void initPrometheusMetrics();
    int startPrometheusExposer(std::string metricHostPort);
    void incrementMessage(std::string protocol, std::string direction);
    void incrementError(std::string protocol, std::string direction);
    void incrementReconnection(std::string protocol, std::string direction);

    int getMessageCount(std::string protocol, std::string direction);
    int getErrorCount(std::string protocol, std::string direction);
    int getReconnectionCount(std::string protocol, std::string direction);

  private:
    std::shared_ptr<prometheus::Registry> m_registry;
    std::unique_ptr<prometheus::Exposer> m_exposer;
    prometheus::Family<prometheus::Counter> *m_api_msg_counter;
    prometheus::Family<prometheus::Counter> *m_api_error_counter;
    prometheus::Family<prometheus::Counter> *m_api_reconnection_counter;
    std::map<LabelKey, prometheus::Counter *> m_message_counters;
    std::map<LabelKey, prometheus::Counter *> m_error_counters;
    std::map<LabelKey, prometheus::Counter *> m_reconnection_counters;
};

#endif // TRAFFIC_REPORTER_HPP