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
    double maxAgeSeconds;
};

enum class TelemetryStatus {
    Nominal,
    Warning,
    Critical,
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
           std::isfinite(reading.maxAgeSeconds) &&
           reading.maxAgeSeconds >= 0.0;
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

    if (reading.value < reading.criticalMinimum ||
        reading.value > reading.criticalMaximum) {
        return TelemetryStatus::Critical;
    }

    if (reading.value < reading.warningMinimum ||
        reading.value > reading.warningMaximum) {
        return TelemetryStatus::Warning;
    }

    return TelemetryStatus::Nominal;
}

std::string statusLabel(TelemetryStatus status) {
    switch (status) {
        case TelemetryStatus::Nominal:
            return "NOMINAL";
        case TelemetryStatus::Warning:
            return "WARNING";
        case TelemetryStatus::Critical:
            return "CRITICAL";
        case TelemetryStatus::Stale:
            return "STALE";
        case TelemetryStatus::MissingData:
            return "NO DATA";
        case TelemetryStatus::InvalidTimestamp:
            return "BAD TIMESTAMP";
        case TelemetryStatus::InvalidConfiguration:
            return "CONFIG ERROR";
    }

    return "UNKNOWN";
}

int main() {
    const std::vector<TelemetryReading> readings = {
        {"Altitude", 18250.0, 0.0, 25000.0, -500.0, 27000.0, "m", 0.4, 2.0},
        {"Velocity", 1240.0, 0.0, 1800.0, -100.0, 2000.0, "m/s", 0.7, 2.0},
        {"Temperature", 91.5, -40.0, 85.0, -55.0, 100.0, "C", 0.3, 5.0},
        {"Pressure", 238.0, 150.0, 300.0, 125.0, 325.0, "kPa", 6.2, 5.0},
        {"Battery Voltage", 33.5, 24.0, 30.0, 22.0, 32.0, "V", 1.1, 5.0},
        {"Fuel Level", std::numeric_limits<double>::quiet_NaN(), 0.0, 100.0, -1.0, 101.0, "%", 0.8, 5.0},
        {"Guidance Quality", 98.0, 90.0, 100.0, 80.0, 105.0, "%", -0.2, 2.0}
    };

    std::cout << "TelemetryGuard - Vehicle Health Check\n\n";

    int warningCount = 0;
    int criticalCount = 0;
    int staleCount = 0;
    int missingDataCount = 0;
    int invalidTimestampCount = 0;
    int configurationErrorCount = 0;

    for (const auto& reading : readings) {
        const TelemetryStatus status = evaluateReading(reading);

        std::cout << std::left << std::setw(18) << reading.channel
                  << std::setw(10);

        if (status == TelemetryStatus::MissingData) {
            std::cout << "N/A";
        } else {
            std::cout << reading.value;
        }

        std::cout << std::setw(8) << reading.unit
                  << std::setw(14) << statusLabel(status)
                  << "age=" << reading.ageSeconds << "s\n";

        if (status == TelemetryStatus::Warning) {
            ++warningCount;
        } else if (status == TelemetryStatus::Critical) {
            ++criticalCount;
        } else if (status == TelemetryStatus::Stale) {
            ++staleCount;
        } else if (status == TelemetryStatus::MissingData) {
            ++missingDataCount;
        } else if (status == TelemetryStatus::InvalidTimestamp) {
            ++invalidTimestampCount;
        } else if (status == TelemetryStatus::InvalidConfiguration) {
            ++configurationErrorCount;
        }
    }

    std::cout << "\nWarnings: " << warningCount << '\n'
              << "Critical alerts: " << criticalCount << '\n'
              << "Stale readings: " << staleCount << '\n'
              << "Missing readings: " << missingDataCount << '\n'
              << "Invalid timestamps: " << invalidTimestampCount << '\n'
              << "Configuration errors: " << configurationErrorCount << '\n';

    return (warningCount == 0 && criticalCount == 0 &&
            staleCount == 0 && missingDataCount == 0 &&
            invalidTimestampCount == 0 &&
            configurationErrorCount == 0)
               ? 0
               : 1;
}
