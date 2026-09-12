#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

struct SaturationAnalysis {
    std::size_t saturated_samples{};
    std::size_t longest_saturated_run{};
    double saturation_rate{};
    bool persistent{};
};

inline SaturationAnalysis analyze_saturation(
    const std::vector<double>& samples,
    double lower_limit,
    double upper_limit,
    std::size_t persistent_run_threshold = 3) {
    if (samples.empty()) throw std::invalid_argument("samples must not be empty");
    if (lower_limit >= upper_limit) throw std::invalid_argument("lower_limit must be below upper_limit");
    if (persistent_run_threshold == 0) throw std::invalid_argument("persistent_run_threshold must be positive");

    std::size_t saturated = 0;
    std::size_t current_run = 0;
    std::size_t longest_run = 0;
    for (double value : samples) {
        const bool is_saturated = value <= lower_limit || value >= upper_limit;
        if (is_saturated) {
            ++saturated;
            ++current_run;
            if (current_run > longest_run) longest_run = current_run;
        } else {
            current_run = 0;
        }
    }

    return {
        saturated,
        longest_run,
        static_cast<double>(saturated) / static_cast<double>(samples.size()),
        longest_run >= persistent_run_threshold,
    };
}
