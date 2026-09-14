#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct SpikeAnalysis {
    std::size_t spike_count;
    double spike_percent;
    double median;
    double median_absolute_deviation;
    bool degraded;
};

inline double median_of(std::vector<double> values) {
    if (values.empty()) {
        throw std::invalid_argument("values cannot be empty");
    }
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    if (values.size() % 2 == 0) {
        return (values[middle - 1] + values[middle]) / 2.0;
    }
    return values[middle];
}

inline SpikeAnalysis analyze_spikes(
    const std::vector<double>& values,
    double threshold = 6.0,
    double degraded_percent = 10.0
) {
    if (values.size() < 3) {
        throw std::invalid_argument("at least three samples are required");
    }
    if (threshold <= 0.0 || degraded_percent < 0.0 || degraded_percent > 100.0) {
        throw std::invalid_argument("invalid spike detection settings");
    }
    for (double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("samples must be finite");
        }
    }

    const double center = median_of(values);
    std::vector<double> deviations;
    deviations.reserve(values.size());
    for (double value : values) {
        deviations.push_back(std::fabs(value - center));
    }
    const double mad = median_of(deviations);

    std::size_t spikes = 0;
    if (mad > 0.0) {
        for (double value : values) {
            const double modified_z = 0.6745 * std::fabs(value - center) / mad;
            if (modified_z > threshold) {
                ++spikes;
            }
        }
    } else {
        for (double value : values) {
            if (value != center) {
                ++spikes;
            }
        }
    }

    const double percent = 100.0 * static_cast<double>(spikes) / static_cast<double>(values.size());
    return {spikes, percent, center, mad, percent >= degraded_percent};
}
