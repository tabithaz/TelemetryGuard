#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

struct FreshnessResult {
    double maximum_age_seconds;
    double average_age_seconds;
    double stale_percentage;
    const char* status;
};

inline FreshnessResult analyze_freshness(
    const std::vector<double>& ages_seconds,
    double stale_threshold_seconds,
    double critical_stale_percentage = 25.0) {
    if (ages_seconds.empty()) {
        throw std::invalid_argument("ages_seconds must not be empty");
    }
    if (stale_threshold_seconds <= 0.0 || critical_stale_percentage < 0.0 || critical_stale_percentage > 100.0) {
        throw std::invalid_argument("freshness thresholds are invalid");
    }

    double total_age = 0.0;
    std::size_t stale_count = 0;
    double maximum_age = 0.0;

    for (double age : ages_seconds) {
        if (age < 0.0) {
            throw std::invalid_argument("telemetry age cannot be negative");
        }
        total_age += age;
        maximum_age = std::max(maximum_age, age);
        if (age > stale_threshold_seconds) {
            ++stale_count;
        }
    }

    const double stale_percentage =
        100.0 * static_cast<double>(stale_count) / static_cast<double>(ages_seconds.size());

    const char* status = "FRESH";
    if (stale_percentage >= critical_stale_percentage && stale_count > 0) {
        status = "CRITICAL";
    } else if (stale_count > 0) {
        status = "STALE";
    }

    return {
        maximum_age,
        total_age / static_cast<double>(ages_seconds.size()),
        stale_percentage,
        status,
    };
}
