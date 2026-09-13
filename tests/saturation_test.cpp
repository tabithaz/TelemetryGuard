#include "../src/saturation.hpp"
#include <cmath>
#include <iostream>
#include <limits>

int main() {
    const auto nominal = analyze_saturation({1.0, 2.0, 3.0}, 0.0, 10.0);
    if (nominal.saturated_samples != 0 || nominal.persistent) return 1;

    const auto persistent = analyze_saturation({1.0, 10.0, 11.0, 12.0, 2.0}, 0.0, 10.0, 3);
    if (persistent.saturated_samples != 3 || persistent.longest_saturated_run != 3 || !persistent.persistent) return 2;
    if (std::abs(persistent.saturation_rate - 0.6) > 1e-9) return 3;

    try {
        (void)analyze_saturation({1.0}, 10.0, 0.0);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    try {
        (void)analyze_saturation({1.0, std::numeric_limits<double>::quiet_NaN()}, 0.0, 10.0);
        return 5;
    } catch (const std::invalid_argument&) {
    }

    try {
        (void)analyze_saturation({1.0}, -std::numeric_limits<double>::infinity(), 10.0);
        return 6;
    } catch (const std::invalid_argument&) {
    }

    try {
        (void)analyze_saturation({1.0}, 0.0, std::numeric_limits<double>::infinity());
        return 7;
    } catch (const std::invalid_argument&) {
    }

    std::cout << "saturation analysis checks passed\n";
    return 0;
}
