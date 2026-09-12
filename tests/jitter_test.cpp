#include "../src/jitter.hpp"

#include <cmath>
#include <iostream>
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

    std::cout << "jitter analysis checks passed\n";
    return 0;
}
