#include "oscillation.hpp"

#include <limits>

int main() {
    const auto steady = analyze_oscillation({1.0, 2.0, 3.0, 4.0});
    if (steady.degraded || steady.reversals != 0) return 1;

    const auto oscillating = analyze_oscillation({1.0, 3.0, 1.0, 3.0, 1.0});
    if (!oscillating.degraded || oscillating.reversals != 3 ||
        oscillating.reversal_rate_percent != 100.0) return 2;

    const auto filtered = analyze_oscillation({10.0, 10.01, 10.0, 10.01, 11.0}, 0.05);
    if (filtered.degraded || filtered.transitions != 0) return 3;

    bool rejected = false;
    try {
        analyze_oscillation({1.0, std::numeric_limits<double>::infinity(), 2.0});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) return 4;
    return 0;
}
