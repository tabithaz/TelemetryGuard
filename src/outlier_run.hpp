#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct OutlierRunReport {
    std::size_t outlier_samples;
    std::size_t longest_outlier_run;
    double outlier_percent;
    bool degraded;
};

inline OutlierRunReport analyze_outlier_runs(
    const std::vector<double>& values,
    double lower_bound,
    double upper_bound,
    std::size_t sustained_run_threshold = 3,
    double degraded_percent = 10.0) {
    if (values.empty()) {
        throw std::invalid_argument("values cannot be empty");
    }
    if (!std::isfinite(lower_bound) || !std::isfinite(upper_bound) || lower_bound >= upper_bound) {
        throw std::invalid_argument("bounds must be finite and ordered");
    }
    if (sustained_run_threshold == 0 || degraded_percent < 0.0 || degraded_percent > 100.0) {
        throw std::invalid_argument("invalid outlier settings");
    }

    std::size_t outliers = 0;
    std::size_t current_run = 0;
    std::size_t longest_run = 0;

    for (double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("samples must be finite");
        }

        if (value < lower_bound || value > upper_bound) {
            ++outliers;
            ++current_run;
            if (current_run > longest_run) {
                longest_run = current_run;
            }
        } else {
            current_run = 0;
        }
    }

    const double outlier_percent =
        100.0 * static_cast<double>(outliers) / static_cast<double>(values.size());
    const bool degraded = longest_run >= sustained_run_threshold || outlier_percent >= degraded_percent;

    return {outliers, longest_run, outlier_percent, degraded};
}
