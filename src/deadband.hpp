#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct DeadbandReport {
    std::size_t transitions;
    std::size_t suppressed_transitions;
    double suppressed_percent;
    double largest_suppressed_delta;
    bool degraded;
};

inline DeadbandReport analyze_deadband(
    const std::vector<double>& values,
    double deadband,
    double warning_percent = 60.0
) {
    if (values.size() < 2) {
        throw std::invalid_argument("at least two samples are required");
    }
    if (!std::isfinite(deadband) || deadband <= 0.0 ||
        !std::isfinite(warning_percent) || warning_percent < 0.0 || warning_percent > 100.0) {
        throw std::invalid_argument("invalid deadband settings");
    }

    std::size_t suppressed = 0;
    double largest_suppressed = 0.0;
    for (std::size_t i = 1; i < values.size(); ++i) {
        if (!std::isfinite(values[i]) || !std::isfinite(values[i - 1])) {
            throw std::invalid_argument("samples must be finite");
        }
        const double delta = std::fabs(values[i] - values[i - 1]);
        if (delta > 0.0 && delta < deadband) {
            ++suppressed;
            if (delta > largest_suppressed) {
                largest_suppressed = delta;
            }
        }
    }

    const std::size_t transitions = values.size() - 1;
    const double suppressed_percent =
        100.0 * static_cast<double>(suppressed) / static_cast<double>(transitions);

    return {
        transitions,
        suppressed,
        suppressed_percent,
        largest_suppressed,
        suppressed_percent >= warning_percent
    };
}
