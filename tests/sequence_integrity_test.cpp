#include "../src/sequence_integrity.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
    const auto nominal = analyze_sequence_integrity({100, 101, 102, 103});
    if (nominal.missing_packets != 0 || nominal.degraded) {
        std::cerr << "nominal sequence should not be degraded\n";
        return 1;
    }

    const auto gapped = analyze_sequence_integrity({10, 11, 14, 15}, 20.0);
    if (gapped.observed_packets != 4 || gapped.missing_packets != 2) {
        std::cerr << "packet accounting mismatch\n";
        return 1;
    }
    if (std::fabs(gapped.loss_percent - 33.3333333333) > 0.001 || !gapped.degraded) {
        std::cerr << "packet loss calculation mismatch\n";
        return 1;
    }

    try {
        (void)analyze_sequence_integrity({5, 5, 6});
        std::cerr << "non-increasing sequence should fail\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    try {
        (void)analyze_sequence_integrity({1, 2}, 101.0);
        std::cerr << "invalid threshold should fail\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
