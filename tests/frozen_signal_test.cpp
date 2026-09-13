#include "../src/frozen_signal.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

int main() {
    const auto healthy = analyze_frozen_signal({1.0, 2.0, 3.0, 4.0}, 0.01, 3);
    if (healthy.frozen) {
        return 1;
    }

    const auto frozen = analyze_frozen_signal({1.0, 2.0, 2.0, 2.0, 2.0, 3.0}, 0.01, 3);
    if (!frozen.frozen || frozen.longest_run != 4) {
        return 2;
    }

    const auto near_constant = analyze_frozen_signal({5.0, 5.001, 5.002, 6.0}, 0.01, 3);
    if (!near_constant.frozen) {
        return 3;
    }

    try {
        analyze_frozen_signal({}, 0.1, 3);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    try {
        analyze_frozen_signal(
            {1.0, std::numeric_limits<double>::quiet_NaN(), 1.0},
            0.1,
            3
        );
        return 5;
    } catch (const std::invalid_argument&) {
    }

    try {
        analyze_frozen_signal(
            {1.0, 1.0, 1.0},
            std::numeric_limits<double>::infinity(),
            3
        );
        return 6;
    } catch (const std::invalid_argument&) {
    }

    std::cout << "frozen signal checks passed\n";
    return 0;
}
