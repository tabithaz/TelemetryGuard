#include <cmath>
#include <iostream>

#include "../src/rate_of_change.hpp"

namespace {
bool close_enough(double left, double right) {
    return std::abs(left - right) < 1e-9;
}
}

int main() {
    const auto rising = telemetry::analyze_rate_of_change(100.0, 130.0, 10.0, 2.5);
    if (!close_enough(rising.per_second, 3.0) || !rising.rapid_change) {
        std::cerr << "rising rate-of-change classification failed\n";
        return 1;
    }

    const auto falling = telemetry::analyze_rate_of_change(50.0, 40.0, 5.0, 3.0);
    if (!close_enough(falling.per_second, -2.0) || falling.rapid_change) {
        std::cerr << "falling rate-of-change classification failed\n";
        return 1;
    }

    try {
        telemetry::analyze_rate_of_change(1.0, 2.0, 0.0, 1.0);
        std::cerr << "zero elapsed time should be rejected\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
