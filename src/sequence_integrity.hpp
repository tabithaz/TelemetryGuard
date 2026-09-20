#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct SequenceIntegrity {
    std::size_t observed_packets;
    std::size_t missing_packets;
    std::size_t gap_events;
    std::size_t largest_gap_packets;
    double average_gap_packets;
    double median_gap_packets;
    double burst_loss_percent;
    double loss_percent;
    bool degraded;
};

inline SequenceIntegrity analyze_sequence_integrity(
    const std::vector<std::uint64_t>& sequence_numbers,
    double degraded_loss_percent = 1.0) {
    if (!std::isfinite(degraded_loss_percent) ||
        degraded_loss_percent < 0.0 || degraded_loss_percent > 100.0) {
        throw std::invalid_argument("degraded loss threshold must be finite and between 0 and 100");
    }

    if (sequence_numbers.empty()) {
        return {0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, false};
    }

    std::size_t missing = 0;
    std::size_t gap_events = 0;
    std::size_t largest_gap = 0;
    std::vector<std::size_t> gap_sizes;
    for (std::size_t i = 1; i < sequence_numbers.size(); ++i) {
        if (sequence_numbers[i] <= sequence_numbers[i - 1]) {
            throw std::invalid_argument("sequence numbers must be strictly increasing");
        }
        const auto gap = static_cast<std::size_t>(
            sequence_numbers[i] - sequence_numbers[i - 1] - 1);
        missing += gap;
        if (gap > 0) {
            ++gap_events;
            gap_sizes.push_back(gap);
        }
        largest_gap = std::max(largest_gap, gap);
    }

    const std::size_t expected = sequence_numbers.size() + missing;
    const double average_gap = gap_events == 0
        ? 0.0
        : static_cast<double>(missing) / static_cast<double>(gap_events);
    double median_gap = 0.0;
    if (!gap_sizes.empty()) {
        std::sort(gap_sizes.begin(), gap_sizes.end());
        const std::size_t middle = gap_sizes.size() / 2;
        median_gap = gap_sizes.size() % 2 == 1
            ? static_cast<double>(gap_sizes[middle])
            : (static_cast<double>(gap_sizes[middle - 1]) +
               static_cast<double>(gap_sizes[middle])) / 2.0;
    }
    const double burst_loss_percent = missing == 0
        ? 0.0
        : (static_cast<double>(largest_gap) / static_cast<double>(missing)) * 100.0;
    const double loss_percent = expected == 0
        ? 0.0
        : (static_cast<double>(missing) / static_cast<double>(expected)) * 100.0;

    return {
        sequence_numbers.size(),
        missing,
        gap_events,
        largest_gap,
        average_gap,
        median_gap,
        burst_loss_percent,
        loss_percent,
        missing > 0 && loss_percent >= degraded_loss_percent,
    };
}
