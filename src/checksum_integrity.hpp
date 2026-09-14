#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct ChecksumIntegrityReport {
    std::size_t total_packets;
    std::size_t failed_packets;
    double failure_rate_percent;
    bool degraded;
};

inline std::uint8_t xor_checksum(const std::vector<std::uint8_t>& payload) {
    std::uint8_t checksum = 0;
    for (const auto byte : payload) {
        checksum ^= byte;
    }
    return checksum;
}

inline ChecksumIntegrityReport analyze_checksum_integrity(
    const std::vector<std::vector<std::uint8_t>>& payloads,
    const std::vector<std::uint8_t>& expected_checksums,
    double degraded_failure_rate_percent = 5.0) {
    if (payloads.size() != expected_checksums.size() || payloads.empty()) {
        throw std::invalid_argument("payloads and checksums must be non-empty and equal length");
    }
    if (degraded_failure_rate_percent < 0.0 || degraded_failure_rate_percent > 100.0) {
        throw std::invalid_argument("failure threshold must be between 0 and 100");
    }

    std::size_t failures = 0;
    for (std::size_t i = 0; i < payloads.size(); ++i) {
        if (xor_checksum(payloads[i]) != expected_checksums[i]) {
            ++failures;
        }
    }

    const double rate = 100.0 * static_cast<double>(failures) /
                        static_cast<double>(payloads.size());
    return {payloads.size(), failures, rate, rate >= degraded_failure_rate_percent};
}
