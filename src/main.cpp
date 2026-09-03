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
};

bool isWithinLimits(const TelemetryReading& reading) {
    return reading.value >= reading.minimum && reading.value <= reading.maximum;
}

int main() {
    const std::vector<TelemetryReading> readings = {
        {"Altitude", 18250.0, 0.0, 25000.0, "m"},
        {"Velocity", 1240.0, 0.0, 1800.0, "m/s"},
        {"Temperature", 91.5, -40.0, 85.0, "C"},
        {"Pressure", 238.0, 150.0, 300.0, "kPa"},
        {"Battery Voltage", 27.8, 24.0, 30.0, "V"}
    };

    std::cout << "TelemetryGuard - Vehicle Health Check\n\n";

    int alertCount = 0;

    for (const auto& reading : readings) {
        const bool healthy = isWithinLimits(reading);

        std::cout << std::left << std::setw(18) << reading.channel
                  << std::setw(10) << reading.value
                  << std::setw(8) << reading.unit
                  << (healthy ? "NOMINAL" : "ALERT") << '\n';

        if (!healthy) {
            ++alertCount;
        }
    }

    std::cout << "\nAlerts detected: " << alertCount << '\n';
    return alertCount == 0 ? 0 : 1;
}
