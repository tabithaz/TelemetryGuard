#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct SlewRateReport {
    double max_rate_per_second;
    double mean_rate_per_second;
    std::size_t violations;
    double violation_rate_percent;
    bool degraded;
};

inline SlewRateReport analyze_slew_rate(
    const std::vector<double>& values,
    const std::vector<double>& timestamps,
    double max_allowed_rate,
    double max_violation_percent = 5.0) {
    if (values.size() < 2 || values.size() != timestamps.size()) {
        throw std::invalid_argument("values and timestamps must have matching samples");
    }
    if (!(max_allowed_rate > 0.0) || max_violation_percent < 0.0 || max_violation_percent > 100.0) {
        throw std::invalid_argument("invalid slew rate thresholds");
    }

    double rate_sum = 0.0;
    double max_rate = 0.0;
    std::size_t violations = 0;

    for (std::size_t i = 1; i < values.size(); ++i) {
        if (!std::isfinite(values[i]) || !std::isfinite(values[i - 1]) ||
            !std::isfinite(timestamps[i]) || !std::isfinite(timestamps[i - 1])) {
            throw std::invalid_argument("samples must be finite");
        }
        const double elapsed = timestamps[i] - timestamps[i - 1];
        if (!(elapsed > 0.0)) {
            throw std::invalid_argument("timestamps must be strictly increasing");
        }
        const double rate = std::fabs(values[i] - values[i - 1]) / elapsed;
        rate_sum += rate;
        if (rate > max_rate) max_rate = rate;
        if (rate > max_allowed_rate) ++violations;
    }

    const double intervals = static_cast<double>(values.size() - 1);
    const double violation_rate = static_cast<double>(violations) / intervals * 100.0;
    return {
        max_rate,
        rate_sum / intervals,
        violations,
        violation_rate,
        violation_rate > max_violation_percent,
    };
}
