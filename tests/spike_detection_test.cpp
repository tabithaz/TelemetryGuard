#include "../src/spike_detection.hpp"

#include <iostream>
#include <limits>
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


    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    for (double invalid_threshold : {nan, infinity, -infinity, 0.0, -1.0}) {
        try {
            analyze_spikes({1.0, 2.0, 3.0}, invalid_threshold);
            return 5;
        } catch (const std::invalid_argument&) {
        }
    }
    for (double invalid_percent : {nan, infinity, -infinity, -1.0, 101.0}) {
        try {
            analyze_spikes({1.0, 2.0, 3.0}, 6.0, invalid_percent);
            return 6;
        } catch (const std::invalid_argument&) {
        }
    }
    for (double invalid_sample : {nan, infinity, -infinity}) {
        try {
            analyze_spikes({1.0, invalid_sample, 3.0});
            return 7;
        } catch (const std::invalid_argument&) {
        }
    }
    const auto zero_percent = analyze_spikes({5.0, 5.0, 5.0}, 6.0, 0.0);
    if (!zero_percent.degraded || zero_percent.spike_count != 0) return 8;
    const auto hundred_percent = analyze_spikes({5.0, 5.0, 5.0}, 6.0, 100.0);
    if (hundred_percent.degraded || hundred_percent.spike_count != 0) return 9;

    std::cout << "spike detection checks passed\n";
    return 0;
}
