#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct DriftResult {
    double slope_per_sample;
    double total_change;
    double r_squared;
    bool sustained_drift;
};

inline DriftResult analyze_sensor_drift(
    const std::vector<double>& values,
    double slope_threshold,
    double minimum_r_squared = 0.8
) {
    if (values.size() < 3) {
        throw std::invalid_argument("at least three samples are required");
    }
    if (!std::isfinite(slope_threshold) || slope_threshold < 0.0) {
        throw std::invalid_argument("slope_threshold must be finite and non-negative");
    }
    if (!std::isfinite(minimum_r_squared) || minimum_r_squared < 0.0 || minimum_r_squared > 1.0) {
        throw std::invalid_argument("minimum_r_squared must be between 0 and 1");
    }
    for (double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("samples must be finite");
        }
    }

    const long double n = static_cast<long double>(values.size());
    const long double mean_x = (n - 1.0L) / 2.0L;
    long double mean_y = 0.0L;
    for (double value : values) mean_y += static_cast<long double>(value) / n;

    long double covariance = 0.0L;
    long double variance_x = 0.0L;
    long double variance_y = 0.0L;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const long double dx = static_cast<long double>(i) - mean_x;
        const long double dy = static_cast<long double>(values[i]) - mean_y;
        covariance += dx * dy;
        variance_x += dx * dx;
        variance_y += dy * dy;
    }

    const long double slope = covariance / variance_x;
    long double r_squared = 0.0L;
    if (variance_y > 0.0L) {
        r_squared = (covariance / variance_x) * (covariance / variance_y);
    }
    r_squared = std::clamp(r_squared, 0.0L, 1.0L);

    const long double total_change = static_cast<long double>(values.back()) -
                                     static_cast<long double>(values.front());
    const bool sustained = std::fabs(slope) >= slope_threshold && r_squared >= minimum_r_squared;
    return {static_cast<double>(slope), static_cast<double>(total_change),
            static_cast<double>(r_squared), sustained};
}
