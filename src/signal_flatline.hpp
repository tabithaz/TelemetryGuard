#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct FlatlineReport {
    std::size_t samples;
    std::size_t longest_run;
    double longest_duration_seconds;
    bool degraded;
};

inline FlatlineReport analyze_flatline(
    const std::vector<double>& values,
    const std::vector<double>& timestamps,
    double tolerance,
    double max_duration_seconds) {
    if (values.size() != timestamps.size() || values.empty()) {
        throw std::invalid_argument("values and timestamps must be non-empty and equal length");
    }
    if (tolerance < 0.0 || max_duration_seconds <= 0.0) {
        throw std::invalid_argument("flatline thresholds are invalid");
    }

    std::size_t run_start = 0;
    std::size_t longest_run = 1;
    double longest_duration = 0.0;

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!std::isfinite(values[i]) || !std::isfinite(timestamps[i])) {
            throw std::invalid_argument("samples must be finite");
        }
        if (i > 0 && timestamps[i] <= timestamps[i - 1]) {
            throw std::invalid_argument("timestamps must be strictly increasing");
        }
        if (i == 0) {
            continue;
        }

        if (std::abs(values[i] - values[i - 1]) > tolerance) {
            run_start = i;
        }

        const std::size_t run_length = i - run_start + 1;
        const double duration = timestamps[i] - timestamps[run_start];
        if (run_length > longest_run) {
            longest_run = run_length;
        }
        if (duration > longest_duration) {
            longest_duration = duration;
        }
    }

    return {values.size(), longest_run, longest_duration,
            longest_duration >= max_duration_seconds};
}
