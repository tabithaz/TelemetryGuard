#include "../src/jitter.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

int main() {
    const auto stable = analyze_jitter({0.0, 100.0, 201.0, 300.0}, 100.0, 5.0);
    if (!near(stable.mean_interval_ms, 100.0) || stable.degraded) return 1;

    const auto noisy = analyze_jitter({0.0, 100.0, 230.0, 330.0, 470.0}, 100.0, 10.0);
    if (!near(noisy.max_jitter_ms, 40.0) || !noisy.degraded) return 2;

    try {
        analyze_jitter({0.0, 0.0}, 100.0, 5.0);
        return 3;
    } catch (const std::invalid_argument&) {
    }

    try {
        analyze_jitter({0.0, std::numeric_limits<double>::infinity()}, 100.0, 5.0);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    try {
        analyze_jitter({0.0, 100.0}, std::numeric_limits<double>::quiet_NaN(), 5.0);
        return 5;
    } catch (const std::invalid_argument&) {
    }

    try {
        analyze_jitter({0.0, 100.0}, 100.0, std::numeric_limits<double>::infinity());
        return 6;
    } catch (const std::invalid_argument&) {
    }

    const double large = 1e308;
    const auto large_nominal = analyze_jitter({-large, 0.0, large}, large, 0.0);
    if (!std::isfinite(large_nominal.mean_interval_ms) ||
        large_nominal.mean_interval_ms != large ||
        large_nominal.mean_absolute_jitter_ms != 0.0 || large_nominal.degraded) return 7;

    const auto large_jitter = analyze_jitter({-large, 0.0, large}, 1.0, 0.0);
    if (!std::isfinite(large_jitter.mean_absolute_jitter_ms) ||
        large_jitter.mean_absolute_jitter_ms != large ||
        large_jitter.max_jitter_ms != large || !large_jitter.degraded) return 8;

    try {
        const double maximum = std::numeric_limits<double>::max();
        (void)analyze_jitter({-maximum, maximum}, 1.0, 0.0);
        return 9;
    } catch (const std::overflow_error&) {
    }

    std::cout << "jitter analysis checks passed\n";
    return 0;
}
