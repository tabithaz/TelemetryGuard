#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct TimestampDriftReport {
    std::size_t intervals;
    double mean_drift_seconds;
    double max_absolute_drift_seconds;
    std::size_t violating_intervals;
    bool degraded;
};

inline TimestampDriftReport analyze_timestamp_drift(
    const std::vector<double>& timestamps,
    double expected_interval_seconds,
    double tolerance_seconds) {
    if (timestamps.size() < 2) {
        throw std::invalid_argument("at least two timestamps are required");
    }
    if (!std::isfinite(expected_interval_seconds) || expected_interval_seconds <= 0.0 ||
        !std::isfinite(tolerance_seconds) || tolerance_seconds < 0.0) {
        throw std::invalid_argument("interval and tolerance must be finite and valid");
    }

    double drift_sum = 0.0;
    double max_absolute_drift = 0.0;
    std::size_t violations = 0;
    for (std::size_t i = 1; i < timestamps.size(); ++i) {
        if (!std::isfinite(timestamps[i - 1]) || !std::isfinite(timestamps[i]) ||
            timestamps[i] <= timestamps[i - 1]) {
            throw std::invalid_argument("timestamps must be finite and strictly increasing");
        }
        const double observed = timestamps[i] - timestamps[i - 1];
        const double drift = observed - expected_interval_seconds;
        const double absolute_drift = std::fabs(drift);
        drift_sum += drift;
        if (absolute_drift > max_absolute_drift) {
            max_absolute_drift = absolute_drift;
        }
        if (absolute_drift > tolerance_seconds) {
            ++violations;
        }
    }

    const auto intervals = timestamps.size() - 1;
    return {
        intervals,
        drift_sum / static_cast<double>(intervals),
        max_absolute_drift,
        violations,
        violations > 0,
    };
}
