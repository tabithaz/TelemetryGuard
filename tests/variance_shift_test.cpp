#include "../src/variance_shift.hpp"

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

    return 0;
}
