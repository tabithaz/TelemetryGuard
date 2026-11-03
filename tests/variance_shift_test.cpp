#include "../src/variance_shift.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

int main() {
    const auto stable = analyze_variance_shift(
        {9.8, 10.0, 10.2, 10.0},
        {9.7, 10.0, 10.3, 10.0});
    if (stable.degraded) return 1;

    const auto noisy = analyze_variance_shift(
        {9.9, 10.0, 10.1, 10.0},
        {8.0, 12.0, 7.5, 12.5});
    if (!noisy.degraded || noisy.ratio < 2.0) return 2;

    const auto flat_baseline = analyze_variance_shift(
        {5.0, 5.0, 5.0, 5.0},
        {4.9, 5.1, 4.9, 5.1});
    if (!flat_baseline.degraded) return 3;

    try {
        analyze_variance_shift({1.0}, {1.0, 2.0});
        return 4;
    } catch (const std::invalid_argument&) {
    }

    const double large = 1e308;
    const double stable_large = population_stddev({large, large, large});
    if (stable_large != 0.0) return 5;

    const double wide_large = population_stddev({-large, large});
    if (!std::isfinite(wide_large) || std::abs(wide_large / large - 1.0) > 1e-12) return 6;

    const auto large_shift = analyze_variance_shift(
        {-1e307, 1e307}, {-large, large}, 2.0);
    if (!large_shift.degraded || !std::isfinite(large_shift.ratio) ||
        std::abs(large_shift.ratio - 10.0) > 1e-12) return 7;

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    for (double threshold : {nan, infinity}) {
        try {
            analyze_variance_shift({1.0, 2.0}, {1.0, 3.0}, threshold);
            return 8;
        } catch (const std::invalid_argument&) {
        }
    }
    for (double floor : {nan, infinity}) {
        try {
            analyze_variance_shift({1.0, 2.0}, {1.0, 3.0}, 2.0, floor);
            return 9;
        } catch (const std::invalid_argument&) {
        }
    }

    return 0;
}
