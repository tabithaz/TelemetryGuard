#include "../src/spike_detection.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    const auto stable = analyze_spikes({10.0, 10.1, 9.9, 10.0, 10.2, 9.8, 10.1}, 6.0, 20.0);
    if (stable.spike_count != 0 || stable.degraded) return 1;

    const auto spiky = analyze_spikes({10.0, 10.1, 9.9, 10.0, 75.0, 9.8, 10.1}, 6.0, 10.0);
    if (spiky.spike_count != 1 || !spiky.degraded) return 2;

    const auto flat_with_spike = analyze_spikes({5.0, 5.0, 5.0, 20.0, 5.0}, 6.0, 10.0);
    if (flat_with_spike.spike_count != 1) return 3;

    try {
        analyze_spikes({1.0, 2.0});
        return 4;
    } catch (const std::invalid_argument&) {
    }

    std::cout << "spike detection checks passed\n";
    return 0;
}
