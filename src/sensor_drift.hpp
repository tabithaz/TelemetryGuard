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

    // Normalize differences instead of multiplying raw readings. This avoids
    // overflow on platforms where long double is no wider than double.
    double scale = 0.0;
    for (double value : values) scale = std::max(scale, std::abs(value));
    if (scale == 0.0) return {0.0, 0.0, 0.0, false};

    const double reference = values.front();
    const auto normalized_delta = [reference, scale](double value) {
        const double difference = value - reference;
        return std::isfinite(difference)
            ? difference / scale
            : value / scale - reference / scale;
    };

    const double n = static_cast<double>(values.size());
    const double mean_x = (n - 1.0) / 2.0;
    double mean_y = 0.0;
    for (double value : values) mean_y += normalized_delta(value) / n;

    double covariance = 0.0;
    double variance_x = 0.0;
    double variance_y = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const double dx = static_cast<double>(i) - mean_x;
        const double dy = normalized_delta(values[i]) - mean_y;
        covariance += dx * dy;
        variance_x += dx * dx;
        variance_y += dy * dy;
    }

    const double slope = (covariance / variance_x) * scale;
    double r_squared = 0.0;
    if (variance_y > 0.0) {
        r_squared = (covariance / variance_x) * (covariance / variance_y);
    }
    r_squared = std::clamp(r_squared, 0.0, 1.0);

    const double total_change = values.back() - values.front();
    const bool sustained = std::fabs(slope) >= slope_threshold && r_squared >= minimum_r_squared;
    return {slope, total_change, r_squared, sustained};
}
