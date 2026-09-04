#include <iomanip>
#include <iostream>
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
    OutOfRange,
    Stale
};

TelemetryStatus evaluateReading(const TelemetryReading& reading) {
    if (reading.ageSeconds > reading.maxAgeSeconds) {
        return TelemetryStatus::Stale;
    }

    if (reading.value < reading.minimum || reading.value > reading.maximum) {
        return TelemetryStatus::OutOfRange;
    }

    return TelemetryStatus::Nominal;
}

std::string statusLabel(TelemetryStatus status) {
    switch (status) {
        case TelemetryStatus::Nominal:
            return "NOMINAL";
        case TelemetryStatus::OutOfRange:
            return "ALERT";
        case TelemetryStatus::Stale:
            return "STALE";
    }

    return "UNKNOWN";
}

int main() {
    const std::vector<TelemetryReading> readings = {
        {"Altitude", 18250.0, 0.0, 25000.0, "m", 0.4, 2.0},
        {"Velocity", 1240.0, 0.0, 1800.0, "m/s", 0.7, 2.0},
        {"Temperature", 91.5, -40.0, 85.0, "C", 0.3, 5.0},
        {"Pressure", 238.0, 150.0, 300.0, "kPa", 6.2, 5.0},
        {"Battery Voltage", 27.8, 24.0, 30.0, "V", 1.1, 5.0}
    };

    std::cout << "TelemetryGuard - Vehicle Health Check\n\n";

    int alertCount = 0;
    int staleCount = 0;

    for (const auto& reading : readings) {
        const TelemetryStatus status = evaluateReading(reading);

        std::cout << std::left << std::setw(18) << reading.channel
                  << std::setw(10) << reading.value
                  << std::setw(8) << reading.unit
                  << std::setw(10) << statusLabel(status)
                  << "age=" << reading.ageSeconds << "s\n";

        if (status == TelemetryStatus::OutOfRange) {
            ++alertCount;
        } else if (status == TelemetryStatus::Stale) {
            ++staleCount;
        }
    }

    std::cout << "\nOut-of-range alerts: " << alertCount << '\n'
              << "Stale readings: " << staleCount << '\n';

    return (alertCount == 0 && staleCount == 0) ? 0 : 1;
}
