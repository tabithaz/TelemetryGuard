#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct OscillationReport {
    std::size_t transitions;
    std::size_t reversals;
    double reversal_rate_percent;
    bool degraded;
};

inline OscillationReport analyze_oscillation(
    const std::vector<double>& samples,
    double minimum_step = 0.0,
    double degraded_reversal_rate_percent = 70.0) {
    if (!std::isfinite(minimum_step) || minimum_step < 0.0 ||
        !std::isfinite(degraded_reversal_rate_percent) ||
        degraded_reversal_rate_percent < 0.0 || degraded_reversal_rate_percent > 100.0) {
        throw std::invalid_argument("invalid oscillation thresholds");
    }
    for (double value : samples) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("samples must be finite");
        }
    }
    if (samples.size() < 3) {
        return {0, 0, 0.0, false};
    }

    std::size_t transitions = 0;
    std::size_t reversals = 0;
    int previous_direction = 0;
    for (std::size_t i = 1; i < samples.size(); ++i) {
        const double delta = samples[i] - samples[i - 1];
        if (std::abs(delta) <= minimum_step) {
            continue;
        }
        const int direction = delta > 0.0 ? 1 : -1;
        if (previous_direction != 0) {
            ++transitions;
            if (direction != previous_direction) {
                ++reversals;
            }
        }
        previous_direction = direction;
    }

    const double rate = transitions == 0 ? 0.0 :
        static_cast<double>(reversals) / static_cast<double>(transitions) * 100.0;
    return {transitions, reversals, rate,
            transitions > 0 && rate >= degraded_reversal_rate_percent};
}
