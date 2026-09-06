#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

struct TelemetryReading {
    std::string channel;
    double value;
    double minimum;
    double maximum;
    std::string unit;
    double ageSeconds;
    double maxAgeSeconds;
};

enum class TelemetryStatus {
    Nominal,
    Warning,
    Critical,
    Stale,
    MissingData
};

TelemetryStatus evaluateReading(const TelemetryReading& reading) {
    if (!std::isfinite(reading.value)) {
        return TelemetryStatus::MissingData;
    }

    if (reading.ageSeconds > reading.maxAgeSeconds) {
        return TelemetryStatus::Stale;
    }

    if (reading.value < reading.minimum || reading.value > reading.maximum) {
        const double operatingRange = reading.maximum - reading.minimum;
        const double criticalMargin = operatingRange * 0.10;

        if (reading.value < reading.minimum - criticalMargin ||
            reading.value > reading.maximum + criticalMargin) {
            return TelemetryStatus::Critical;
        }

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
    }

    return "UNKNOWN";
}

int main() {
    const std::vector<TelemetryReading> readings = {
        {"Altitude", 18250.0, 0.0, 25000.0, "m", 0.4, 2.0},
        {"Velocity", 1240.0, 0.0, 1800.0, "m/s", 0.7, 2.0},
        {"Temperature", 91.5, -40.0, 85.0, "C", 0.3, 5.0},
        {"Pressure", 238.0, 150.0, 300.0, "kPa", 6.2, 5.0},
        {"Battery Voltage", 33.5, 24.0, 30.0, "V", 1.1, 5.0},
        {"Fuel Level", std::numeric_limits<double>::quiet_NaN(), 0.0, 100.0, "%", 0.8, 5.0}
    };

    std::cout << "TelemetryGuard - Vehicle Health Check\n\n";

    int warningCount = 0;
    int criticalCount = 0;
    int staleCount = 0;
    int missingDataCount = 0;

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
                  << std::setw(10) << statusLabel(status)
                  << "age=" << reading.ageSeconds << "s\n";

        if (status == TelemetryStatus::Warning) {
            ++warningCount;
        } else if (status == TelemetryStatus::Critical) {
            ++criticalCount;
        } else if (status == TelemetryStatus::Stale) {
            ++staleCount;
        } else if (status == TelemetryStatus::MissingData) {
            ++missingDataCount;
        }
    }

    std::cout << "\nWarnings: " << warningCount << '\n'
              << "Critical alerts: " << criticalCount << '\n'
              << "Stale readings: " << staleCount << '\n'
              << "Missing readings: " << missingDataCount << '\n';

    return (warningCount == 0 && criticalCount == 0 &&
            staleCount == 0 && missingDataCount == 0)
               ? 0
               : 1;
}
