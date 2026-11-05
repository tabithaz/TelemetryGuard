#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>

namespace telemetry {

struct RateOfChange {
    double per_second;
    double absolute_per_second;
    bool rapid_change;
};

inline RateOfChange analyze_rate_of_change(
    double previous_value,
    double current_value,
    double elapsed_seconds,
    double rapid_change_threshold_per_second
) {
    if (!std::isfinite(previous_value) || !std::isfinite(current_value)) {
        throw std::invalid_argument("telemetry values must be finite");
    }
    if (!std::isfinite(elapsed_seconds) || elapsed_seconds <= 0.0) {
        throw std::invalid_argument("elapsed_seconds must be finite and positive");
    }
    if (!std::isfinite(rapid_change_threshold_per_second) ||
        rapid_change_threshold_per_second < 0.0) {
        throw std::invalid_argument("rapid change threshold must be finite and non-negative");
    }

    // Keep intermediate differences in extended precision when the final rate fits.
    const long double delta = static_cast<long double>(current_value) -
                              static_cast<long double>(previous_value);
    const long double wide_rate = delta / static_cast<long double>(elapsed_seconds);
    if (!std::isfinite(wide_rate) ||
        std::abs(wide_rate) > static_cast<long double>(std::numeric_limits<double>::max())) {
        throw std::overflow_error("rate of change exceeds finite range");
    }
    const double rate = static_cast<double>(wide_rate);
    const double absolute_rate = std::abs(rate);
    return RateOfChange{
        rate,
        absolute_rate,
        absolute_rate >= rapid_change_threshold_per_second,
    };
}

}  // namespace telemetry
