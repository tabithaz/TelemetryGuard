#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct SensorAgreementAnalysis {
    std::size_t samples;
    double mean_absolute_difference;
    double max_absolute_difference;
    double disagreement_rate;
    bool degraded;
};

inline SensorAgreementAnalysis analyze_sensor_agreement(
    const std::vector<double>& primary,
    const std::vector<double>& redundant,
    double tolerance) {
    if (tolerance < 0.0) {
        throw std::invalid_argument("tolerance must be non-negative");
    }
    if (primary.size() != redundant.size()) {
        throw std::invalid_argument("sensor streams must have equal length");
    }
    if (primary.empty()) {
        return {0, 0.0, 0.0, 0.0, false};
    }

    double total_difference = 0.0;
    double max_difference = 0.0;
    std::size_t disagreements = 0;

    for (std::size_t i = 0; i < primary.size(); ++i) {
        const double difference = std::abs(primary[i] - redundant[i]);
        total_difference += difference;
        max_difference = std::max(max_difference, difference);
        if (difference > tolerance) {
            ++disagreements;
        }
    }

    const double disagreement_rate =
        static_cast<double>(disagreements) / static_cast<double>(primary.size());

    return {
        primary.size(),
        total_difference / static_cast<double>(primary.size()),
        max_difference,
        disagreement_rate,
        disagreement_rate >= 0.20,
    };
}
