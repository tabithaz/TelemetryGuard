#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct StuckBitReport {
    std::size_t samples;
    std::size_t bits_observed;
    std::size_t stuck_bits;
    double stuck_bit_percent;
    bool degraded;
};

inline StuckBitReport analyze_stuck_bits(
    const std::vector<std::uint32_t>& samples,
    std::size_t bits_observed = 16,
    double max_stuck_percent = 50.0) {
    if (samples.size() < 2) {
        throw std::invalid_argument("at least two samples are required");
    }
    if (bits_observed == 0 || bits_observed > 32) {
        throw std::invalid_argument("bits_observed must be between 1 and 32");
    }
    if (!std::isfinite(max_stuck_percent) || max_stuck_percent < 0.0 || max_stuck_percent > 100.0) {
        throw std::invalid_argument("max_stuck_percent must be between 0 and 100");
    }

    const std::uint32_t mask = bits_observed == 32
        ? 0xFFFFFFFFu
        : ((std::uint32_t{1} << bits_observed) - 1u);
    std::uint32_t changed_bits = 0u;
    const std::uint32_t first = samples.front() & mask;
    for (std::size_t i = 1; i < samples.size(); ++i) {
        changed_bits |= first ^ (samples[i] & mask);
    }

    std::size_t changing_count = 0;
    for (std::size_t bit = 0; bit < bits_observed; ++bit) {
        if ((changed_bits & (std::uint32_t{1} << bit)) != 0u) {
            ++changing_count;
        }
    }

    const std::size_t stuck_bits = bits_observed - changing_count;
    const double stuck_percent = 100.0 * static_cast<double>(stuck_bits) / static_cast<double>(bits_observed);
    return {samples.size(), bits_observed, stuck_bits, stuck_percent, stuck_percent > max_stuck_percent};
}
