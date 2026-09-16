#include "../src/bias_shift.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

int main() {
    const auto stable = analyze_bias_shift({10.0, 10.2, 9.8, 10.1, 10.0, 9.9}, 3, 0.0, 20.0);
    if (stable.degraded) return 1;

    const auto shifted = analyze_bias_shift({10.0, 10.1, 9.9, 14.0, 14.1, 13.9}, 3, 0.0, 20.0);
    if (!shifted.degraded || shifted.absolute_shift < 3.9) return 2;

    const auto threshold = analyze_bias_shift({0.0, 0.0, 1.0, 1.0}, 2, 0.0, 10.0, 10.0);
    if (!threshold.degraded || std::fabs(threshold.shift_percent_of_range - 10.0) > 1e-9) return 3;

    try {
        analyze_bias_shift({1.0, 2.0, 3.0}, 2, 0.0, 10.0);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
