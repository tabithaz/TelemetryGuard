#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

struct RangeUtilizationReport {
    double minimum;
    double maximum;
    double observed_span;
    double utilization_ratio;
    bool degraded;
};

inline RangeUtilizationReport analyze_range_utilization(
    const std::vector<double>& samples,
    double expected_minimum,
    double expected_maximum,
    double minimum_utilization_ratio = 0.10) {
    if (samples.empty()) {
        throw std::invalid_argument("at least one sample is required");
    }
    if (!std::isfinite(expected_minimum) || !std::isfinite(expected_maximum) ||
        expected_minimum >= expected_maximum) {
        throw std::invalid_argument("expected range must be finite and increasing");
    }
    if (minimum_utilization_ratio < 0.0 || minimum_utilization_ratio > 1.0) {
        throw std::invalid_argument("minimum utilization ratio must be between zero and one");
    }

    double minimum = samples.front();
    double maximum = samples.front();
    for (double sample : samples) {
        if (!std::isfinite(sample)) {
            throw std::invalid_argument("samples must be finite");
        }
        minimum = std::min(minimum, sample);
        maximum = std::max(maximum, sample);
    }

    const double observed_span = maximum - minimum;
    const double expected_span = expected_maximum - expected_minimum;
    const double utilization = observed_span / expected_span;

    return {
        minimum,
        maximum,
        observed_span,
        utilization,
        utilization < minimum_utilization_ratio,
    };
}
