#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct NoiseFloorReport {
    double mean_delta;
    double rms_delta;
    double quiet_percent;
    bool degraded;
};

inline NoiseFloorReport analyze_noise_floor(
    const std::vector<double>& values,
    double quiet_threshold,
    double max_rms_delta,
    double max_quiet_percent = 95.0) {
    if (values.size() < 2) throw std::invalid_argument("at least two samples are required");
    if (!std::isfinite(quiet_threshold) || !std::isfinite(max_rms_delta) || !std::isfinite(max_quiet_percent) ||
        quiet_threshold < 0.0 || max_rms_delta <= 0.0 || max_quiet_percent < 0.0 || max_quiet_percent > 100.0)
        throw std::invalid_argument("invalid noise-floor settings");

    double sum = 0.0;
    double squared_sum = 0.0;
    std::size_t quiet = 0;
    for (std::size_t i = 1; i < values.size(); ++i) {
        if (!std::isfinite(values[i]) || !std::isfinite(values[i - 1]))
            throw std::invalid_argument("samples must be finite");
        const double delta = std::fabs(values[i] - values[i - 1]);
        sum += delta;
        squared_sum += delta * delta;
        if (delta <= quiet_threshold) ++quiet;
    }

    const double intervals = static_cast<double>(values.size() - 1);
    const double mean_delta = sum / intervals;
    const double rms_delta = std::sqrt(squared_sum / intervals);
    const double quiet_percent = 100.0 * static_cast<double>(quiet) / intervals;
    return {mean_delta, rms_delta, quiet_percent, rms_delta > max_rms_delta || quiet_percent > max_quiet_percent};
}
