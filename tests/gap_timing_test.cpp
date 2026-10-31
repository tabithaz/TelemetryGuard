#include "../src/gap_timing.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

int main() {
    const auto healthy = analyze_gap_timing({0.0, 1.0, 2.0, 3.0}, 1.0);
    if (healthy.degraded || healthy.gap_count != 0) return 1;

    const auto gapped = analyze_gap_timing({0.0, 1.0, 5.0, 6.0}, 1.0, 2.0, 20.0);
    if (!gapped.degraded || gapped.gap_count != 1 || gapped.maximum_interval_seconds != 4.0) return 2;

    const auto tolerant = analyze_gap_timing({0.0, 1.0, 2.5, 3.5}, 1.0, 2.0, 10.0);
    if (tolerant.degraded) return 3;

    try {
        analyze_gap_timing({0.0, 0.0, 1.0}, 1.0);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    try {
        analyze_gap_timing({0.0}, 1.0);
        return 5;
    } catch (const std::invalid_argument&) {
    }


    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    const double maximum = std::numeric_limits<double>::max();
    const auto rejects = [](auto&& check) {
        try {
            check();
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };

    if (!rejects([&] { analyze_gap_timing({0.0, 1.0}, nan); }) ||
        !rejects([&] { analyze_gap_timing({0.0, 1.0}, infinity); }) ||
        !rejects([&] { analyze_gap_timing({0.0, 1.0}, 1.0, nan); }) ||
        !rejects([&] { analyze_gap_timing({0.0, 1.0}, 1.0, infinity); }) ||
        !rejects([&] { analyze_gap_timing({0.0, 1.0}, 1.0, 2.0, nan); }) ||
        !rejects([&] { analyze_gap_timing({0.0, 1.0}, 1.0, 2.0, infinity); })) {
        return 6;
    }

    if (!rejects([&] { analyze_gap_timing({0.0, 1.0}, maximum, 2.0); }) ||
        !rejects([&] { analyze_gap_timing({-maximum, maximum}, 1.0); })) {
        return 7;
    }

    const double large_interval = maximum * 0.75;
    const auto large = analyze_gap_timing({-large_interval, 0.0, large_interval}, 1.0);
    if (!std::isfinite(large.mean_interval_seconds) ||
        std::abs(large.mean_interval_seconds / large_interval - 1.0) > 1e-12 ||
        large.maximum_interval_seconds != large_interval ||
        large.gap_count != 2 || large.gap_rate_percent != 100.0 || !large.degraded) {
        return 8;
    }

    const auto mixed = analyze_gap_timing({-large_interval, 0.0, maximum * 0.5}, 1.0);
    if (!std::isfinite(mixed.mean_interval_seconds) ||
        std::abs(mixed.mean_interval_seconds / maximum - 0.625) > 1e-12 ||
        mixed.maximum_interval_seconds != large_interval) {
        return 9;
    }

    return 0;
}
