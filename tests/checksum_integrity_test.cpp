#include "../src/checksum_integrity.hpp"

#include <stdexcept>
#include <vector>

int main() {
    const std::vector<std::vector<std::uint8_t>> payloads{{1, 2, 3}, {4, 5}, {9, 9}};
    const std::vector<std::uint8_t> valid_checksums{
        xor_checksum(payloads[0]),
        xor_checksum(payloads[1]),
        xor_checksum(payloads[2]),
    };

    const auto healthy = analyze_checksum_integrity(payloads, valid_checksums);
    if (healthy.failed_packets != 0 || healthy.degraded) {
        return 1;
    }

    auto corrupted = valid_checksums;
    corrupted[1] ^= 0x01;
    const auto degraded = analyze_checksum_integrity(payloads, corrupted, 20.0);
    if (degraded.failed_packets != 1 || !degraded.degraded) {
        return 2;
    }

    try {
        analyze_checksum_integrity({}, {});
        return 3;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
