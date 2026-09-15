#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct DropoutAnalysis {
    std::size_t dropout_events;
    std::size_t total_missing_packets;
    std::size_t longest_dropout;
    double missing_packet_percent;
    bool sustained_dropout;
};

inline DropoutAnalysis analyze_dropouts(
    const std::vector<std::uint64_t>& sequence_numbers,
    std::size_t sustained_threshold = 3) {
    if (sustained_threshold == 0) {
        throw std::invalid_argument("sustained threshold must be positive");
    }

    std::size_t dropout_events = 0;
    std::size_t total_missing = 0;
    std::size_t longest_dropout = 0;

    for (std::size_t i = 1; i < sequence_numbers.size(); ++i) {
        if (sequence_numbers[i] <= sequence_numbers[i - 1]) {
            throw std::invalid_argument("sequence numbers must be strictly increasing");
        }
        const auto missing = static_cast<std::size_t>(sequence_numbers[i] - sequence_numbers[i - 1] - 1);
        if (missing > 0) {
            ++dropout_events;
            total_missing += missing;
            if (missing > longest_dropout) {
                longest_dropout = missing;
            }
        }
    }

    const std::size_t expected_packets = sequence_numbers.size() + total_missing;
    const double missing_percent = expected_packets == 0
        ? 0.0
        : 100.0 * static_cast<double>(total_missing) / static_cast<double>(expected_packets);

    return {
        dropout_events,
        total_missing,
        longest_dropout,
        missing_percent,
        longest_dropout >= sustained_threshold,
    };
}
