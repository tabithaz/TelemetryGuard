#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct BiasShiftReport {
    double baseline_mean;
    double recent_mean;
    double absolute_shift;
    double shift_percent_of_range;
    bool degraded;
};

inline BiasShiftReport analyze_bias_shift(
    const std::vector<double>& samples,
    std::size_t baseline_samples,
    double expected_min,
    double expected_max,
    double warning_percent = 10.0) {
    if (samples.size() < 4 || baseline_samples < 2 || baseline_samples >= samples.size()) {
        throw std::invalid_argument("baseline and recent windows require at least two samples");
    }
    if (!std::isfinite(expected_min) || !std::isfinite(expected_max) || expected_min >= expected_max ||
        warning_percent < 0.0) {
        throw std::invalid_argument("invalid bias shift settings");
    }

    double baseline_sum = 0.0;
    double recent_sum = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (!std::isfinite(samples[i])) {
            throw std::invalid_argument("samples must be finite");
        }
        if (i < baseline_samples) {
            baseline_sum += samples[i];
        } else {
            recent_sum += samples[i];
        }
    }

    const auto recent_count = samples.size() - baseline_samples;
    const double baseline_mean = baseline_sum / static_cast<double>(baseline_samples);
    const double recent_mean = recent_sum / static_cast<double>(recent_count);
    const double shift = std::fabs(recent_mean - baseline_mean);
    const double range = expected_max - expected_min;
    const double shift_percent = shift / range * 100.0;

    return {baseline_mean, recent_mean, shift, shift_percent, shift_percent >= warning_percent};
}
