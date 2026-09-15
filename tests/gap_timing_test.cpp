#include "../src/gap_timing.hpp"

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

    return 0;
}
