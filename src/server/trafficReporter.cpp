#include "trafficReporter.hpp"
#include <sstream>

std::string TrafficReporter::generateReport() const {

    std::ostringstream report;
    report << "Traffic Report:\n";
    report << "Messages Received: " << m_stats.messageCount.load() << "\n";
    report << "Errors: " << m_stats.errorCount.load() << "\n";
    report << "Reconnections: " << m_stats.reconnectionCount.load() << "\n";
    return report.str();
}