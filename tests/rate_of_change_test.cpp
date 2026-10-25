#include <cmath>
#include <iostream>
#include <limits>

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

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();

    try {
        telemetry::analyze_rate_of_change(nan, 2.0, 1.0, 1.0);
        std::cerr << "non-finite telemetry value should be rejected\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    try {
        telemetry::analyze_rate_of_change(1.0, 2.0, infinity, 1.0);
        std::cerr << "non-finite elapsed time should be rejected\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    try {
        telemetry::analyze_rate_of_change(1.0, 2.0, 1.0, nan);
        std::cerr << "non-finite rate threshold should be rejected\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    const double max_value = std::numeric_limits<double>::max();
    try {
        telemetry::analyze_rate_of_change(-max_value, max_value, 2.0, 1.0);
        std::cerr << "overflowing telemetry difference should be rejected\n";
        return 1;
    } catch (const std::overflow_error&) {
    }

    try {
        telemetry::analyze_rate_of_change(0.0, max_value, 0.5, 1.0);
        std::cerr << "overflowing rate should be rejected\n";
        return 1;
    } catch (const std::overflow_error&) {
    }

    const auto finite_large = telemetry::analyze_rate_of_change(0.0, max_value, max_value, 1.0);
    if (!close_enough(finite_large.per_second, 1.0) || !finite_large.rapid_change) {
        std::cerr << "large but finite rate should remain valid\n";
        return 1;
    }

    return 0;
}
