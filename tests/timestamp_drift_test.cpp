#include "timestamp_drift.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    const auto nominal = analyze_timestamp_drift({0.0, 1.0, 2.01, 3.0}, 1.0, 0.05);
    if (nominal.degraded || nominal.violating_intervals != 0 ||
        std::fabs(nominal.max_absolute_drift_seconds - 0.01) > 0.0001) {
        std::cerr << "nominal timing should remain healthy\n";
        return 1;
    }

    const auto drifting = analyze_timestamp_drift({0.0, 1.0, 2.2, 3.45}, 1.0, 0.10);
    if (!drifting.degraded || drifting.violating_intervals != 2 ||
        std::fabs(drifting.max_absolute_drift_seconds - 0.25) > 0.0001) {
        std::cerr << "timestamp drift was not detected\n";
        return 1;
    }

    try {
        analyze_timestamp_drift({0.0, 1.0, 1.0}, 1.0, 0.1);
        std::cerr << "duplicate timestamps should be rejected\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
