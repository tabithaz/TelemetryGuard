#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct CounterRegressionReport {
    std::size_t samples;
    std::size_t regressions;
    double largest_regression;
    double regression_rate_percent;
    bool degraded;
};

inline CounterRegressionReport analyze_counter_regressions(
    const std::vector<double>& values,
    double reset_floor = 0.0,
    double reset_ceiling = 1.0,
    double degraded_rate_percent = 1.0) {
    if (values.empty()) throw std::invalid_argument("counter series must not be empty");
    if (!std::isfinite(reset_floor) || !std::isfinite(reset_ceiling) || reset_floor > reset_ceiling ||
        !std::isfinite(degraded_rate_percent) || degraded_rate_percent < 0.0 || degraded_rate_percent > 100.0)
        throw std::invalid_argument("invalid counter regression threshold");
    std::size_t regressions = 0;
    double largest_regression = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!std::isfinite(values[i])) throw std::invalid_argument("counter samples must be finite");
        if (i == 0 || values[i] >= values[i - 1]) continue;
        const bool expected_reset = values[i] >= reset_floor && values[i] <= reset_ceiling;
        if (!expected_reset) {
            ++regressions;
            largest_regression = std::max(largest_regression, values[i - 1] - values[i]);
        }
    }
    const std::size_t transitions = values.size() > 1 ? values.size() - 1 : 0;
    const double rate = transitions ? 100.0 * static_cast<double>(regressions) / static_cast<double>(transitions) : 0.0;
    return {values.size(), regressions, largest_regression, rate, rate >= degraded_rate_percent};
}
