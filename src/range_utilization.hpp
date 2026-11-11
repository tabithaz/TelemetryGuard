#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
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
    if (!std::isfinite(minimum_utilization_ratio) ||
        minimum_utilization_ratio < 0.0 || minimum_utilization_ratio > 1.0) {
        throw std::invalid_argument("minimum utilization ratio must be finite and between zero and one");
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

    // Calculate in double without assuming extended-precision long double
    // is available. Half-width ratios avoid overflow when only the
    // *expected* span exceeds the double range.
    const double observed_span = maximum - minimum;
    if (!std::isfinite(observed_span)) {
        throw std::overflow_error("observed span exceeds finite range");
    }
    const double expected_span = expected_maximum - expected_minimum;
    const double utilization = std::isfinite(expected_span)
        ? observed_span / expected_span
        : (maximum / 2.0 - minimum / 2.0) /
          (expected_maximum / 2.0 - expected_minimum / 2.0);
    if (!std::isfinite(utilization)) {
        throw std::overflow_error("utilization ratio exceeds finite range");
    }

    return {
        minimum,
        maximum,
        observed_span,
        utilization,
        utilization < minimum_utilization_ratio,
    };
}
