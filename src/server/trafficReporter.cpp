#include "trafficReporter.hpp"
#include <iostream>
#include <sstream>

using namespace prometheus;

TrafficReporter::TrafficReporter()
    : m_registry(nullptr)
    , m_exposer(nullptr)
    , m_api_msg_counter(nullptr)
    , m_api_error_counter(nullptr)
    , m_api_reconnection_counter(nullptr) {

    initPrometheusMetrics();
}

void TrafficReporter::initPrometheusMetrics() {

    m_registry = std::make_shared<Registry>();

    m_api_msg_counter =
        &BuildCounter().Name("observed_messages_total").Help("Number of observed messages").Register(*m_registry);

    m_api_error_counter =
        &BuildCounter().Name("observed_errors_total").Help("Number of observed errors").Register(*m_registry);

    m_api_reconnection_counter = &BuildCounter()
                                      .Name("observed_reconnections_total")
                                      .Help("Number of observed reconnections")
                                      .Register(*m_registry);
}

int TrafficReporter::startPrometheusExposer(const std::string &metricHostPort) {
    // Start the Prometheus HTTP server
    try {
        m_exposer = std::make_unique<prometheus::Exposer>(metricHostPort);
        m_exposer->RegisterCollectable(m_registry);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error initializing Prometheus Exposer: " << e.what() << '\n';
        return -1;
    }
}

void TrafficReporter::incrementMessage(const std::string &protocol, const std::string &direction) {
    // no need for mutex, prometheus is thread safe (atomic counters)
    LabelKey key{protocol, direction};
    auto tmp = m_message_counters.find(key);
    if (tmp == m_message_counters.end()) {
        auto &counter = m_api_msg_counter->Add({{"protocol", protocol}, {"direction", direction}});
        m_message_counters[key] = &counter;
        tmp = m_message_counters.find(key);
    }
    tmp->second->Increment();
}

void TrafficReporter::incrementError(const std::string &protocol, const std::string &direction) {

    LabelKey key{protocol, direction};
    auto tmp = m_error_counters.find(key);
    if (tmp == m_error_counters.end()) {
        auto &counter = m_api_error_counter->Add({{"protocol", protocol}, {"direction", direction}});
        m_error_counters[key] = &counter;
        tmp = m_error_counters.find(key);
    }
    tmp->second->Increment();
}

void TrafficReporter::incrementReconnection(const std::string &protocol, const std::string &direction) {

    LabelKey key{protocol, direction};
    auto tmp = m_reconnection_counters.find(key);
    if (tmp == m_reconnection_counters.end()) {
        auto &counter = m_api_reconnection_counter->Add({{"protocol", protocol}, {"direction", direction}});
        m_reconnection_counters[key] = &counter;
        tmp = m_reconnection_counters.find(key);
    }
    tmp->second->Increment();
}

int TrafficReporter::getErrorCount(const std::string &protocol, const std::string &direction) {
    LabelKey key{protocol, direction};
    auto tmp = m_error_counters.find(key);
    if (tmp == m_error_counters.end()) {
        return 0;
    }
    // static_cast to cast the value from double to int. There is no float usage in the system
    return static_cast<int>(tmp->second->Value());
}

int TrafficReporter::getReconnectionCount(const std::string &protocol, const std::string &direction) {
    LabelKey key{protocol, direction};
    auto tmp = m_reconnection_counters.find(key);
    if (tmp == m_reconnection_counters.end()) {
        return 0;
    }
    return static_cast<int>(tmp->second->Value());
}

int TrafficReporter::getMessageCount(const std::string &protocol, const std::string &direction) {
    LabelKey key{protocol, direction};
    auto tmp = m_message_counters.find(key);
    if (tmp == m_message_counters.end()) {
        return 0;
    }
    return static_cast<int>(tmp->second->Value());
}