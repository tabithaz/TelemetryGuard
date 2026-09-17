#include "../src/slew_rate.hpp"

#include <stdexcept>

int main() {
    const auto nominal = analyze_slew_rate({0.0, 1.0, 2.0, 3.0}, {0.0, 1.0, 2.0, 3.0}, 2.0);
    if (nominal.degraded || nominal.violations != 0) return 1;

    const auto abrupt = analyze_slew_rate({0.0, 1.0, 10.0, 11.0}, {0.0, 1.0, 2.0, 3.0}, 5.0, 20.0);
    if (!abrupt.degraded || abrupt.violations != 1 || abrupt.max_rate_per_second != 9.0) return 2;

    const auto tolerated = analyze_slew_rate({0.0, 1.0, 10.0, 11.0}, {0.0, 1.0, 2.0, 3.0}, 5.0, 40.0);
    if (tolerated.degraded) return 3;

    try {
        analyze_slew_rate({0.0, 1.0}, {1.0, 1.0}, 2.0);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
