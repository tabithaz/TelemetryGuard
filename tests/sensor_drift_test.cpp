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

    const double large = 1e308;
    const auto large_rising = analyze_sensor_drift({-large, 0.0, large}, 1e307);
    if (!large_rising.sustained_drift || !std::isfinite(large_rising.slope_per_sample) ||
        std::abs(large_rising.slope_per_sample / large - 1.0) > 1e-12 ||
        std::abs(large_rising.r_squared - 1.0) > 1e-12) return 6;

    const auto large_stable = analyze_sensor_drift({large, large, large}, 1.0);
    if (large_stable.sustained_drift || large_stable.slope_per_sample != 0.0 ||
        large_stable.r_squared != 0.0) return 7;

    const auto large_offset = analyze_sensor_drift(
        {large, large + 1e293, large + 2e293}, 1e291);
    if (!large_offset.sustained_drift || !std::isfinite(large_offset.r_squared) ||
        large_offset.r_squared < 0.99) return 8;

    std::cout << "sensor drift checks passed\n";
    return 0;
}
