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
        return {0, 0, 0, 0, 0.0, 0.0, false};
    }

    std::size_t missing = 0;
    std::size_t gap_events = 0;
    std::size_t largest_gap = 0;
    for (std::size_t i = 1; i < sequence_numbers.size(); ++i) {
        if (sequence_numbers[i] <= sequence_numbers[i - 1]) {
            throw std::invalid_argument("sequence numbers must be strictly increasing");
        }
        const auto gap = static_cast<std::size_t>(
            sequence_numbers[i] - sequence_numbers[i - 1] - 1);
        missing += gap;
        if (gap > 0) {
            ++gap_events;
        }
        largest_gap = std::max(largest_gap, gap);
    }

    const std::size_t expected = sequence_numbers.size() + missing;
    const double average_gap = gap_events == 0
        ? 0.0
        : static_cast<double>(missing) / static_cast<double>(gap_events);
    const double loss_percent = expected == 0
        ? 0.0
        : (static_cast<double>(missing) / static_cast<double>(expected)) * 100.0;

    return {
        sequence_numbers.size(),
        missing,
        gap_events,
        largest_gap,
        average_gap,
        loss_percent,
        missing > 0 && loss_percent >= degraded_loss_percent,
    };
}
