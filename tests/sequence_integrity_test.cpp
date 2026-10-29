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
        nominal.median_gap_packets != 0.0 || nominal.burst_loss_percent != 0.0 ||
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
        zeroThresholdLoss.gap_events != 1 || zeroThresholdLoss.average_gap_packets != 1.0 ||
        zeroThresholdLoss.median_gap_packets != 1.0 ||
        zeroThresholdLoss.burst_loss_percent != 100.0) {
        std::cerr << "packet loss should degrade at zero threshold\n";
        return 1;
    }

    const auto gapped = analyze_sequence_integrity({10, 11, 14, 15, 20}, 20.0);
    if (gapped.observed_packets != 5 || gapped.missing_packets != 6 ||
        gapped.gap_events != 2 || gapped.largest_gap_packets != 4) {
        std::cerr << "packet accounting mismatch\n";
        return 1;
    }
    if (std::fabs(gapped.average_gap_packets - 3.0) > 0.001 ||
        std::fabs(gapped.median_gap_packets - 3.0) > 0.001) {
        std::cerr << "packet gap statistics mismatch\n";
        return 1;
    }
    if (std::fabs(gapped.burst_loss_percent - 66.6666666667) > 0.001) {
        std::cerr << "burst loss concentration mismatch\n";
        return 1;
    }
    if (std::fabs(gapped.loss_percent - 54.5454545455) > 0.001 || !gapped.degraded) {
        std::cerr << "packet loss calculation mismatch\n";
        return 1;
    }

    const auto unevenGaps = analyze_sequence_integrity({1, 3, 8, 10});
    if (std::fabs(unevenGaps.median_gap_packets - 1.0) > 0.001) {
        std::cerr << "odd packet gap median mismatch\n";
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

    const auto max_sequence = std::numeric_limits<std::uint64_t>::max();
    const auto full_span = analyze_sequence_integrity({0, max_sequence});
    if (!full_span.degraded || !std::isfinite(full_span.loss_percent) ||
        full_span.loss_percent < 99.0 || full_span.loss_percent > 100.0) {
        std::cerr << "full-range packet loss must not wrap expected count\n";
        return 1;
    }

    const auto full_span_with_tail = analyze_sequence_integrity({0, max_sequence - 1, max_sequence});
    if (!full_span_with_tail.degraded || full_span_with_tail.loss_percent < 99.0 ||
        full_span_with_tail.loss_percent > 100.0 || full_span_with_tail.gap_events != 1) {
        std::cerr << "full-range packet loss with contiguous tail was misclassified\n";
        return 1;
    }

    const auto near_max_nominal = analyze_sequence_integrity({max_sequence - 2, max_sequence - 1, max_sequence});
    if (near_max_nominal.degraded || near_max_nominal.loss_percent != 0.0) {
        std::cerr << "contiguous high sequence numbers must remain nominal\n";
        return 1;
    }

    return 0;
}
