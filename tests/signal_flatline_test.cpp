#include "signal_flatline.hpp"

#include <cassert>
#include <stdexcept>

int main() {
    const auto healthy = analyze_flatline(
        {10.0, 10.0, 10.2, 10.4}, {0.0, 1.0, 2.0, 3.0}, 0.05, 2.0);
    assert(!healthy.degraded);
    assert(healthy.longest_run == 2);

    const auto degraded = analyze_flatline(
        {5.0, 5.01, 5.0, 5.01}, {0.0, 1.0, 2.0, 3.0}, 0.05, 2.5);
    assert(degraded.degraded);
    assert(degraded.longest_duration_seconds == 3.0);

    bool rejected = false;
    try {
        (void)analyze_flatline({1.0, 1.0}, {1.0, 1.0}, 0.1, 1.0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
    return 0;
}
