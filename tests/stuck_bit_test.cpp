#include "stuck_bit.hpp"

#include <cassert>
#include <stdexcept>
#include <vector>

int main() {
    const auto healthy = analyze_stuck_bits({0u, 1u, 2u, 3u, 4u, 7u, 8u, 15u}, 4, 25.0);
    assert(healthy.stuck_bits == 0);
    assert(!healthy.degraded);

    const auto degraded = analyze_stuck_bits({0u, 1u, 0u, 1u}, 8, 50.0);
    assert(degraded.stuck_bits == 7);
    assert(degraded.stuck_bit_percent == 87.5);
    assert(degraded.degraded);

    bool rejected = false;
    try {
        analyze_stuck_bits({1u}, 8);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        analyze_stuck_bits({1u, 2u}, 0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    return 0;
}
