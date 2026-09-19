#include "../src/sequence_integrity.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

int main() {
    const auto nominal = analyze_sequence_integrity({100, 101, 102, 103});
    if (nominal.missing_packets != 0 || nominal.gap_events != 0 ||
        nominal.largest_gap_packets != 0 || nominal.average_gap_packets != 0.0 ||
        nominal.degraded) {
        std::cerr << "nominal sequence should not be degraded\n";
        return 1;
    }

    const auto zeroThresholdNominal = analyze_sequence_integrity({20, 21, 22}, 0.0);
    if (zeroThresholdNominal.degraded) {
        std::cerr << "lossless sequence should remain nominal at zero threshold\n";
        return 1;
    }

    const auto zeroThresholdLoss = analyze_sequence_integrity({20, 22}, 0.0);
    if (!zeroThresholdLoss.degraded || zeroThresholdLoss.missing_packets != 1 ||
        zeroThresholdLoss.gap_events != 1 || zeroThresholdLoss.average_gap_packets != 1.0) {
        std::cerr << "packet loss should degrade at zero threshold\n";
        return 1;
    }

    const auto gapped = analyze_sequence_integrity({10, 11, 14, 15, 20}, 20.0);
    if (gapped.observed_packets != 5 || gapped.missing_packets != 6 ||
        gapped.gap_events != 2 || gapped.largest_gap_packets != 4) {
        std::cerr << "packet accounting mismatch\n";
        return 1;
    }
    if (std::fabs(gapped.average_gap_packets - 3.0) > 0.001) {
        std::cerr << "average packet gap calculation mismatch\n";
        return 1;
    }
    if (std::fabs(gapped.loss_percent - 54.5454545455) > 0.001 || !gapped.degraded) {
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

    try {
        (void)analyze_sequence_integrity(
            {1, 2}, std::numeric_limits<double>::quiet_NaN());
        std::cerr << "NaN threshold should fail\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    try {
        (void)analyze_sequence_integrity(
            {1, 2}, std::numeric_limits<double>::infinity());
        std::cerr << "infinite threshold should fail\n";
        return 1;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
