#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct FrozenSignalResult {
    std::size_t longest_run;
    double frozen_percent;
    bool frozen;
};

inline FrozenSignalResult analyze_frozen_signal(
    const std::vector<double>& values,
    double epsilon,
    std::size_t run_threshold
) {
    if (values.empty()) {
        throw std::invalid_argument("values cannot be empty");
    }
    if (epsilon < 0.0 || run_threshold < 2) {
        throw std::invalid_argument("invalid frozen-signal settings");
    }

    std::size_t longest = 1;
    std::size_t current = 1;
    std::size_t frozen_samples = 0;

    for (std::size_t i = 1; i < values.size(); ++i) {
        if (std::fabs(values[i] - values[i - 1]) <= epsilon) {
            ++current;
        } else {
            if (current >= run_threshold) {
                frozen_samples += current;
            }
            current = 1;
        }
        if (current > longest) {
            longest = current;
        }
    }

    if (current >= run_threshold) {
        frozen_samples += current;
    }

    return {
        longest,
        100.0 * static_cast<double>(frozen_samples) / static_cast<double>(values.size()),
        longest >= run_threshold,
    };
}
