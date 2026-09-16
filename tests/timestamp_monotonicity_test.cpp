#include "../src/timestamp_monotonicity.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

int main() {
    const auto healthy = analyze_timestamp_monotonicity({1.0, 2.0, 3.0, 4.0});
    if (healthy.degraded || healthy.regression_count != 0 || healthy.duplicate_rate_percent != 0.0) return 1;

    const auto regression = analyze_timestamp_monotonicity({1.0, 2.0, 1.5, 3.0});
    if (!regression.degraded || regression.regression_count != 1) return 2;

    const auto duplicate = analyze_timestamp_monotonicity({1.0, 1.0, 2.0});
    if (!duplicate.degraded || duplicate.duplicate_count != 1 ||
        std::fabs(duplicate.duplicate_rate_percent - 50.0) > 1e-9) return 3;

    const auto allowed = analyze_timestamp_monotonicity({1.0, 1.0, 2.0}, 1);
    if (allowed.degraded || std::fabs(allowed.duplicate_rate_percent - 50.0) > 1e-9) return 4;

    try {
        analyze_timestamp_monotonicity({1.0, std::numeric_limits<double>::quiet_NaN()});
        return 5;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
