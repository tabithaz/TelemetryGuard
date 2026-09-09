#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

struct TelemetryReading {
    std::string channel;
    double value;
    double warningMinimum;
    double warningMaximum;
    double criticalMinimum;
    double criticalMaximum;
    std::string unit;
    double ageSeconds;
    double warningAgeSeconds;
    double maxAgeSeconds;
};

enum class TelemetryStatus {
    Nominal,
    Warning,
    Critical,
    Aging,
    Stale,
    MissingData,
    InvalidTimestamp,
    InvalidConfiguration
};

bool hasValidConfiguration(const TelemetryReading& reading) {
    return std::isfinite(reading.warningMinimum) &&
           std::isfinite(reading.warningMaximum) &&
           std::isfinite(reading.criticalMinimum) &&
           std::isfinite(reading.criticalMaximum) &&
           reading.criticalMinimum <= reading.warningMinimum &&
           reading.warningMinimum <= reading.warningMaximum &&
           reading.warningMaximum <= reading.criticalMaximum &&
           std::isfinite(reading.warningAgeSeconds) &&
           reading.warningAgeSeconds >= 0.0 &&
           std::isfinite(reading.maxAgeSeconds) &&
           reading.warningAgeSeconds <= reading.maxAgeSeconds;
}

TelemetryStatus evaluateReading(const TelemetryReading& reading) {
    if (!hasValidConfiguration(reading)) {
        return TelemetryStatus::InvalidConfiguration;
    }
    if (!std::isfinite(reading.value)) {
        return TelemetryStatus::MissingData;
    }
    if (!std::isfinite(reading.ageSeconds) || reading.ageSeconds < 0.0) {
        return TelemetryStatus::InvalidTimestamp;
    }
    if (reading.ageSeconds > reading.maxAgeSeconds) {
        return TelemetryStatus::Stale;
    }
    if (reading.value < reading.criticalMinimum || reading.value > reading.criticalMaximum) {
        return TelemetryStatus::Critical;
    }
    if (reading.value < reading.warningMinimum || reading.value > reading.warningMaximum) {
        return TelemetryStatus::Warning;
    }
    if (reading.ageSeconds > reading.warningAgeSeconds) {
        return TelemetryStatus::Aging;
    }
    return TelemetryStatus::Nominal;
}

std::string statusLabel(TelemetryStatus status) {
    switch (status) {
        case TelemetryStatus::Nominal: return "NOMINAL";
        case TelemetryStatus::Warning: return "WARNING";
        case TelemetryStatus::Critical: return "CRITICAL";
        case TelemetryStatus::Aging: return "AGING";
        case TelemetryStatus::Stale: return "STALE";
        case TelemetryStatus::MissingData: return "NO DATA";
        case TelemetryStatus::InvalidTimestamp: return "BAD TIMESTAMP";
        case TelemetryStatus::InvalidConfiguration: return "CONFIG ERROR";
    }
    return "UNKNOWN";
}

int statusPriority(TelemetryStatus status) {
    switch (status) {
        case TelemetryStatus::InvalidConfiguration: return 7;
        case TelemetryStatus::MissingData: return 6;
        case TelemetryStatus::Critical: return 5;
        case TelemetryStatus::Stale: return 4;
        case TelemetryStatus::InvalidTimestamp: return 3;
        case TelemetryStatus::Warning: return 2;
        case TelemetryStatus::Aging: return 1;
        case TelemetryStatus::Nominal: return 0;
    }
    return 0;
}

bool requiresHold(TelemetryStatus status) {
    return status == TelemetryStatus::Critical ||
           status == TelemetryStatus::Stale ||
           status == TelemetryStatus::MissingData ||
           status == TelemetryStatus::InvalidTimestamp ||
           status == TelemetryStatus::InvalidConfiguration;
}

std::string vehicleDisposition(int warningCount, int criticalCount, int agingCount,
                               int staleCount, int missingDataCount,
                               int invalidTimestampCount, int configurationErrorCount) {
    const int blockingIssues = criticalCount + staleCount + missingDataCount +
                               invalidTimestampCount + configurationErrorCount;
    if (blockingIssues > 0) return "HOLD";
    if (warningCount > 0 || agingCount > 0) return "MONITOR";
    return "GO";
}

int vehicleHealthScore(int totalReadings, int warningCount, int criticalCount,
                       int agingCount, int staleCount, int missingDataCount,
                       int invalidTimestampCount, int configurationErrorCount) {
    if (totalReadings <= 0) return 100;

    const int penalty = warningCount * 8 + agingCount * 5 + criticalCount * 25 +
                        staleCount * 20 + missingDataCount * 25 +
                        invalidTimestampCount * 20 + configurationErrorCount * 25;
    return std::max(0, 100 - penalty);
}

double telemetryAvailabilityPercent(int totalReadings, int staleCount,
                                    int missingDataCount, int invalidTimestampCount,
                                    int configurationErrorCount) {
    if (totalReadings <= 0) return 100.0;

    const int unavailableReadings = staleCount + missingDataCount +
                                    invalidTimestampCount + configurationErrorCount;
    const int availableReadings = std::max(0, totalReadings - unavailableReadings);
    return 100.0 * static_cast<double>(availableReadings) /
           static_cast<double>(totalReadings);
}

int main() {
    const std::vector<TelemetryReading> readings = {
        {"Altitude", 18250.0, 0.0, 25000.0, -500.0, 27000.0, "m", 0.4, 1.5, 2.0},
        {"Velocity", 1240.0, 0.0, 1800.0, -100.0, 2000.0, "m/s", 0.7, 1.5, 2.0},
        {"Temperature", 91.5, -40.0, 85.0, -55.0, 100.0, "C", 0.3, 4.0, 5.0},
        {"Pressure", 238.0, 150.0, 300.0, 125.0, 325.0, "kPa", 6.2, 4.0, 5.0},
        {"Battery Voltage", 33.5, 24.0, 30.0, 22.0, 32.0, "V", 1.1, 4.0, 5.0},
        {"Fuel Level", std::numeric_limits<double>::quiet_NaN(), 0.0, 100.0, -1.0, 101.0, "%", 0.8, 4.0, 5.0},
        {"Guidance Quality", 98.0, 90.0, 100.0, 80.0, 105.0, "%", -0.2, 1.5, 2.0},
        {"Navigation Update", 97.0, 90.0, 100.0, 80.0, 105.0, "%", 1.7, 1.5, 2.0}
    };

    std::cout << "TelemetryGuard - Vehicle Health Check\n\n";

    int warningCount = 0;
    int criticalCount = 0;
    int agingCount = 0;
    int staleCount = 0;
    int missingDataCount = 0;
    int invalidTimestampCount = 0;
    int configurationErrorCount = 0;
    int blockingIssueCount = 0;
    int highestPriority = -1;
    std::string priorityChannel = "None";
    TelemetryStatus priorityStatus = TelemetryStatus::Nominal;

    for (const auto& reading : readings) {
        const TelemetryStatus status = evaluateReading(reading);
        std::cout << std::left << std::setw(18) << reading.channel << std::setw(10);
        if (status == TelemetryStatus::MissingData) std::cout << "N/A";
        else std::cout << reading.value;

        std::cout << std::setw(8) << reading.unit << std::setw(14) << statusLabel(status)
                  << "age=" << reading.ageSeconds << "s\n";

        if (status == TelemetryStatus::Warning) ++warningCount;
        else if (status == TelemetryStatus::Critical) ++criticalCount;
        else if (status == TelemetryStatus::Aging) ++agingCount;
        else if (status == TelemetryStatus::Stale) ++staleCount;
        else if (status == TelemetryStatus::MissingData) ++missingDataCount;
        else if (status == TelemetryStatus::InvalidTimestamp) ++invalidTimestampCount;
        else if (status == TelemetryStatus::InvalidConfiguration) ++configurationErrorCount;

        if (requiresHold(status)) ++blockingIssueCount;
        const int priority = statusPriority(status);
        if (priority > highestPriority) {
            highestPriority = priority;
            priorityChannel = reading.channel;
            priorityStatus = status;
        }
    }

    const int totalReadings = static_cast<int>(readings.size());
    const int healthScore = vehicleHealthScore(
        totalReadings, warningCount, criticalCount, agingCount, staleCount,
        missingDataCount, invalidTimestampCount, configurationErrorCount);
    const double availability = telemetryAvailabilityPercent(
        totalReadings, staleCount, missingDataCount, invalidTimestampCount,
        configurationErrorCount);

    std::cout << "\nWarnings: " << warningCount << '\n'
              << "Critical alerts: " << criticalCount << '\n'
              << "Aging readings: " << agingCount << '\n'
              << "Stale readings: " << staleCount << '\n'
              << "Missing readings: " << missingDataCount << '\n'
              << "Invalid timestamps: " << invalidTimestampCount << '\n'
              << "Configuration errors: " << configurationErrorCount << '\n'
              << "Blocking issues: " << blockingIssueCount << '\n'
              << "Priority channel: " << priorityChannel << " ("
              << statusLabel(priorityStatus) << ")\n"
              << "Telemetry availability: " << std::fixed << std::setprecision(1)
              << availability << "%\n"
              << "Vehicle health score: " << healthScore << "/100\n"
              << "Vehicle disposition: "
              << vehicleDisposition(warningCount, criticalCount, agingCount, staleCount,
                                    missingDataCount, invalidTimestampCount,
                                    configurationErrorCount)
              << '\n';

    return (warningCount == 0 && criticalCount == 0 && agingCount == 0 && staleCount == 0 &&
            missingDataCount == 0 && invalidTimestampCount == 0 && configurationErrorCount == 0)
               ? 0
               : 1;
}
