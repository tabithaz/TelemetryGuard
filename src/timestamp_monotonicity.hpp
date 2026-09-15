#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct TimestampMonotonicityReport {
    std::size_t regression_count;
    std::size_t duplicate_count;
    double regression_rate_percent;
    bool degraded;
};

inline TimestampMonotonicityReport analyze_timestamp_monotonicity(
    const std::vector<double>& timestamps,
    std::size_t allowed_duplicates = 0) {
    if (timestamps.size() < 2) {
        throw std::invalid_argument("at least two timestamps are required");
    }

    std::size_t regressions = 0;
    std::size_t duplicates = 0;

    for (std::size_t index = 0; index < timestamps.size(); ++index) {
        if (!std::isfinite(timestamps[index])) {
            throw std::invalid_argument("timestamps must be finite");
        }
        if (index == 0) {
            continue;
        }
        if (timestamps[index] < timestamps[index - 1]) {
            ++regressions;
        } else if (timestamps[index] == timestamps[index - 1]) {
            ++duplicates;
        }
    }

    const auto intervals = static_cast<double>(timestamps.size() - 1);
    const double regression_rate = 100.0 * static_cast<double>(regressions) / intervals;

    return {
        regressions,
        duplicates,
        regression_rate,
        regressions > 0 || duplicates > allowed_duplicates,
    };
}
