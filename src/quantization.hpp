#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct QuantizationReport {
    double mean_step;
    double max_step;
    double repeated_percent;
    bool degraded;
};

inline QuantizationReport analyze_quantization(
    const std::vector<double>& values,
    double expected_resolution,
    double tolerance_ratio = 0.25
) {
    if (values.size() < 2) {
        throw std::invalid_argument("at least two samples are required");
    }
    if (!(expected_resolution > 0.0) || tolerance_ratio < 0.0) {
        throw std::invalid_argument("invalid quantization settings");
    }

    double step_sum = 0.0;
    double max_step = 0.0;
    std::size_t changes = 0;
    std::size_t repeats = 0;
    std::size_t off_grid = 0;

    for (std::size_t i = 1; i < values.size(); ++i) {
        if (!std::isfinite(values[i]) || !std::isfinite(values[i - 1])) {
            throw std::invalid_argument("samples must be finite");
        }

        const double delta = std::fabs(values[i] - values[i - 1]);
        if (delta == 0.0) {
            ++repeats;
            continue;
        }

        ++changes;
        step_sum += delta;
        if (delta > max_step) {
            max_step = delta;
        }

        const double units = delta / expected_resolution;
        if (std::fabs(units - std::round(units)) > tolerance_ratio) {
            ++off_grid;
        }
    }

    const double mean_step = changes
        ? step_sum / static_cast<double>(changes)
        : 0.0;
    const double repeated_percent = 100.0 * static_cast<double>(repeats)
        / static_cast<double>(values.size() - 1);
    const bool degraded = (changes > 0 && off_grid * 4 > changes)
        || repeated_percent >= 80.0;

    return {mean_step, max_step, repeated_percent, degraded};
}
