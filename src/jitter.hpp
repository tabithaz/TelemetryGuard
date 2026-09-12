#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

struct JitterAnalysis {
    double mean_interval_ms;
    double max_jitter_ms;
    double mean_absolute_jitter_ms;
    double jitter_rate_percent;
    bool degraded;
};

inline JitterAnalysis analyze_jitter(
    const std::vector<double>& timestamps_ms,
    double expected_interval_ms,
    double tolerance_ms) {
    if (timestamps_ms.size() < 2) {
        throw std::invalid_argument("at least two timestamps are required");
    }
    if (expected_interval_ms <= 0.0 || tolerance_ms < 0.0) {
        throw std::invalid_argument("invalid interval or tolerance");
    }

    double interval_sum = 0.0;
    double jitter_sum = 0.0;
    double max_jitter = 0.0;
    std::size_t violations = 0;

    for (std::size_t i = 1; i < timestamps_ms.size(); ++i) {
        const double interval = timestamps_ms[i] - timestamps_ms[i - 1];
        if (interval <= 0.0) {
            throw std::invalid_argument("timestamps must be strictly increasing");
        }
        const double jitter = std::abs(interval - expected_interval_ms);
        interval_sum += interval;
        jitter_sum += jitter;
        max_jitter = std::max(max_jitter, jitter);
        if (jitter > tolerance_ms) {
            ++violations;
        }
    }

    const auto count = static_cast<double>(timestamps_ms.size() - 1);
    const double violation_rate = (static_cast<double>(violations) / count) * 100.0;
    return {
        interval_sum / count,
        max_jitter,
        jitter_sum / count,
        violation_rate,
        violation_rate >= 20.0
    };
}
