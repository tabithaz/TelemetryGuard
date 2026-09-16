#pragma once

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

    double mean = 0.0;
    for (double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("samples must be finite");
        }
        mean += value;
    }
    mean /= static_cast<double>(values.size());

    double squared_error = 0.0;
    for (double value : values) {
        const double delta = value - mean;
        squared_error += delta * delta;
    }
    return std::sqrt(squared_error / static_cast<double>(values.size()));
}

inline VarianceShiftReport analyze_variance_shift(
    const std::vector<double>& baseline,
    const std::vector<double>& recent,
    double ratio_threshold = 2.0,
    double minimum_stddev = 1e-9) {
    if (ratio_threshold <= 1.0 || minimum_stddev <= 0.0) {
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
