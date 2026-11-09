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

    const long double wide_observed_span = static_cast<long double>(maximum) -
                                           static_cast<long double>(minimum);
    const long double wide_expected_span = static_cast<long double>(expected_maximum) -
                                           static_cast<long double>(expected_minimum);
    if (wide_observed_span > static_cast<long double>(std::numeric_limits<double>::max())) {
        throw std::overflow_error("observed span exceeds finite range");
    }
    const double observed_span = static_cast<double>(wide_observed_span);
    const long double wide_utilization = wide_observed_span / wide_expected_span;
    if (wide_utilization > static_cast<long double>(std::numeric_limits<double>::max())) {
        throw std::overflow_error("utilization ratio exceeds finite range");
    }
    const double utilization = static_cast<double>(wide_utilization);

    return {
        minimum,
        maximum,
        observed_span,
        utilization,
        utilization < minimum_utilization_ratio,
    };
}
