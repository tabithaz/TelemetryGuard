#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct StepChangeReport {
    std::size_t transitions;
    std::size_t step_changes;
    double largest_step;
    double step_rate_percent;
    bool degraded;
};

inline StepChangeReport analyze_step_changes(
    const std::vector<double>& samples,
    double step_threshold,
    double degraded_rate_percent = 10.0) {
    if (samples.size() < 2) {
        throw std::invalid_argument("at least two samples are required");
    }
    if (!std::isfinite(step_threshold) || step_threshold <= 0.0) {
        throw std::invalid_argument("step threshold must be positive and finite");
    }
    if (!std::isfinite(degraded_rate_percent) || degraded_rate_percent < 0.0 ||
        degraded_rate_percent > 100.0) {
        throw std::invalid_argument("degraded rate must be between 0 and 100");
    }

    std::size_t changes = 0;
    double largest = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (!std::isfinite(samples[i])) {
            throw std::invalid_argument("samples must be finite");
        }
        if (i == 0) {
            continue;
        }
        const double delta = std::fabs(samples[i] - samples[i - 1]);
        if (delta > largest) {
            largest = delta;
        }
        if (delta >= step_threshold) {
            ++changes;
        }
    }

    const std::size_t transitions = samples.size() - 1;
    const double rate = 100.0 * static_cast<double>(changes) /
                        static_cast<double>(transitions);
    return {transitions, changes, largest, rate, rate >= degraded_rate_percent};
}
