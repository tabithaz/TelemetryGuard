#include "../src/deadband.hpp"

#include <stdexcept>

int main() {
    const auto healthy = analyze_deadband({0.0, 1.0, 2.0, 3.0}, 0.5);
    if (healthy.degraded || healthy.suppressed_transitions != 0) return 1;

    const auto degraded = analyze_deadband({1.0, 1.1, 1.2, 1.3, 2.0}, 0.5, 50.0);
    if (!degraded.degraded || degraded.suppressed_transitions != 3) return 2;

    const auto mixed = analyze_deadband({0.0, 0.2, 1.0, 1.1}, 0.5, 80.0);
    if (mixed.degraded || mixed.largest_suppressed_delta < 0.19) return 3;

    try {
        analyze_deadband({1.0}, 0.5);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
