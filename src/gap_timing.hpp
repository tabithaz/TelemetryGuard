#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct GapTimingReport {
    double mean_interval_seconds;
    double maximum_interval_seconds;
    std::size_t gap_count;
    double gap_rate_percent;
    bool degraded;
};

inline GapTimingReport analyze_gap_timing(
    const std::vector<double>& timestamps,
    double expected_interval_seconds,
    double gap_multiplier = 2.0,
    double degraded_gap_rate_percent = 10.0) {
    if (timestamps.size() < 2) {
        throw std::invalid_argument("at least two timestamps are required");
    }
    if (expected_interval_seconds <= 0.0 || gap_multiplier <= 1.0 ||
        degraded_gap_rate_percent < 0.0 || degraded_gap_rate_percent > 100.0) {
        throw std::invalid_argument("invalid gap timing settings");
    }

    double interval_sum = 0.0;
    double maximum_interval = 0.0;
    std::size_t gap_count = 0;
    const double gap_threshold = expected_interval_seconds * gap_multiplier;

    for (std::size_t i = 1; i < timestamps.size(); ++i) {
        if (!std::isfinite(timestamps[i]) || !std::isfinite(timestamps[i - 1]) ||
            timestamps[i] <= timestamps[i - 1]) {
            throw std::invalid_argument("timestamps must be finite and strictly increasing");
        }
        const double interval = timestamps[i] - timestamps[i - 1];
        interval_sum += interval;
        maximum_interval = std::max(maximum_interval, interval);
        if (interval > gap_threshold) {
            ++gap_count;
        }
    }

    const auto interval_count = timestamps.size() - 1;
    const double gap_rate = 100.0 * static_cast<double>(gap_count) /
                            static_cast<double>(interval_count);

    return {
        interval_sum / static_cast<double>(interval_count),
        maximum_interval,
        gap_count,
        gap_rate,
        gap_rate >= degraded_gap_rate_percent && gap_count > 0,
    };
}
