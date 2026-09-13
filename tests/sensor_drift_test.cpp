#include "../src/sensor_drift.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

int main() {
    const auto stable = analyze_sensor_drift({10.0, 10.1, 9.9, 10.0, 10.1}, 0.2);
    if (stable.sustained_drift) return 1;

    const auto rising = analyze_sensor_drift({1.0, 1.5, 2.0, 2.5, 3.0}, 0.4);
    if (!rising.sustained_drift || rising.slope_per_sample < 0.49 || rising.r_squared < 0.99) return 2;

    const auto noisy = analyze_sensor_drift({1.0, 4.0, 0.5, 4.5, 1.0}, 0.2, 0.9);
    if (noisy.sustained_drift) return 3;

    try {
        analyze_sensor_drift({1.0, 2.0}, 0.1);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    try {
        analyze_sensor_drift({1.0, std::numeric_limits<double>::quiet_NaN(), 3.0}, 0.1);
        return 5;
    } catch (const std::invalid_argument&) {
    }

    std::cout << "sensor drift checks passed\n";
    return 0;
}
