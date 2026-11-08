#include "../src/range_utilization.hpp"

#include <stdexcept>
#include <limits>

int main() {
    const auto healthy = analyze_range_utilization({1.0, 4.0, 8.0}, 0.0, 10.0);
    if (healthy.degraded || healthy.utilization_ratio != 0.7) return 1;

    const auto narrow = analyze_range_utilization({5.0, 5.1, 5.2}, 0.0, 10.0);
    if (!narrow.degraded) return 2;

    const auto custom = analyze_range_utilization({2.0, 3.0}, 0.0, 10.0, 0.05);
    if (custom.degraded) return 3;

    try {
        analyze_range_utilization({}, 0.0, 10.0);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity()}) {
        try {
            analyze_range_utilization({1.0, 2.0}, 0.0, 10.0, invalid);
            return 5;
        } catch (const std::invalid_argument&) {
        }
    }

    const double huge = std::numeric_limits<double>::max();
    const auto finite = analyze_range_utilization({-huge / 4, huge / 4},
                                                  -huge, huge, 0.20);
    if (!std::isfinite(finite.utilization_ratio) ||
        finite.utilization_ratio != 0.25 || finite.degraded) return 6;

    try {
        analyze_range_utilization({-huge, huge}, -huge, huge);
        return 7;
    } catch (const std::overflow_error&) {
    }

    return 0;
}
