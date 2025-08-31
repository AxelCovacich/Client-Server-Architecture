#include "trafficReporter.hpp"
#include <sstream>

using namespace prometheus;

TrafficReporter::TrafficReporter()
    : m_api_msg_counter(nullptr)
    , m_api_error_counter(nullptr)
    , m_api_reconnection_counter(nullptr)
    , m_message_counters()
    , m_error_counters()
    , m_reconnection_counters() {
}

void TrafficReporter::initPrometheusMetrics(std::string metricHostPort) {

    Exposer exposer(metricHostPort);

    auto registry = std::make_shared<Registry>();

    m_api_msg_counter =
        &BuildCounter().Name("observed_messages_total").Help("Number of observed messages").Register(*registry);

    m_api_error_counter =
        &BuildCounter().Name("observed_errors_total").Help("Number of observed errors").Register(*registry);

    m_api_reconnection_counter = &BuildCounter()
                                      .Name("observed_reconnections_total")
                                      .Help("Number of observed reconnections")
                                      .Register(*registry);
}

void TrafficReporter::incrementMessage(std::string protocol, std::string direction) {

    LabelKey key{protocol, direction};
    auto tmp = m_message_counters.find(key);
    if (tmp == m_message_counters.end()) {
        auto &counter = m_api_msg_counter->Add({{"protocol", protocol}, {"direction", direction}});
        m_message_counters[key] = &counter;
        tmp = m_message_counters.find(key);
    }
    tmp->second->Increment();
}

void TrafficReporter::incrementError(std::string protocol, std::string direction) {

    LabelKey key{protocol, direction};
    auto tmp = m_error_counters.find(key);
    if (tmp == m_error_counters.end()) {
        auto &counter = m_api_error_counter->Add({{"protocol", protocol}, {"direction", direction}});
        m_error_counters[key] = &counter;
        tmp = m_error_counters.find(key);
    }
    tmp->second->Increment();
}

void TrafficReporter::incrementReconnection(std::string protocol, std::string direction) {

    LabelKey key{protocol, direction};
    auto tmp = m_reconnection_counters.find(key);
    if (tmp == m_reconnection_counters.end()) {
        auto &counter = m_api_reconnection_counter->Add({{"protocol", protocol}, {"direction", direction}});
        m_reconnection_counters[key] = &counter;
        tmp = m_reconnection_counters.find(key);
    }
    tmp->second->Increment();
}

int TrafficReporter::getErrorCount(std::string protocol, std::string direction) {
    LabelKey key{protocol, direction};
    auto tmp = m_error_counters.find(key);
    if (tmp == m_error_counters.end()) {
        return 0;
    }
    // static_cast to cast the value from double to int. There is no float usage in the system
    return static_cast<int>(tmp->second->Value());
}

int TrafficReporter::getReconnectionCount(std::string protocol, std::string direction) {
    LabelKey key{protocol, direction};
    auto tmp = m_reconnection_counters.find(key);
    if (tmp == m_reconnection_counters.end()) {
        return 0;
    }
    return static_cast<int>(tmp->second->Value());
}

int TrafficReporter::getMessageCount(std::string protocol, std::string direction) {
    LabelKey key{protocol, direction};
    auto tmp = m_message_counters.find(key);
    if (tmp == m_message_counters.end()) {
        return 0;
    }
    return static_cast<int>(tmp->second->Value());
}