#include "../src/range_utilization.hpp"

#include <stdexcept>

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

    return 0;
}
