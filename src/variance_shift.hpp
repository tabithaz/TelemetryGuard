#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

struct VarianceShiftReport {
    double baseline_stddev;
    double recent_stddev;
    double ratio;
    bool degraded;
};

inline double population_stddev(const std::vector<double>& values) {
    if (values.size() < 2) {
        throw std::invalid_argument("at least two samples are required");
    }

    long double scale = 0.0L;
    for (double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("samples must be finite");
        }
        scale = std::max(scale, std::fabs(static_cast<long double>(value)));
    }
    if (scale == 0.0L) {
        return 0.0;
    }

    long double mean = 0.0L;
    long double squared_deviations = 0.0L;
    std::size_t count = 0;
    for (double value : values) {
        const long double normalized = static_cast<long double>(value) / scale;
        const long double delta = normalized - mean;
        mean += delta / static_cast<long double>(++count);
        squared_deviations += delta * (normalized - mean);
    }
    const long double variance = std::max(0.0L, squared_deviations / static_cast<long double>(count));
    return static_cast<double>(std::sqrt(variance) * scale);
}

inline VarianceShiftReport analyze_variance_shift(
    const std::vector<double>& baseline,
    const std::vector<double>& recent,
    double ratio_threshold = 2.0,
    double minimum_stddev = 1e-9) {
    if (!std::isfinite(ratio_threshold) || !std::isfinite(minimum_stddev) ||
        ratio_threshold <= 1.0 || minimum_stddev <= 0.0) {
        throw std::invalid_argument("invalid variance shift settings");
    }

    const double baseline_stddev = population_stddev(baseline);
    const double recent_stddev = population_stddev(recent);
    const double denominator = std::max(baseline_stddev, minimum_stddev);
    const double ratio = recent_stddev / denominator;

    return {
        baseline_stddev,
        recent_stddev,
        ratio,
        ratio >= ratio_threshold
    };
}
