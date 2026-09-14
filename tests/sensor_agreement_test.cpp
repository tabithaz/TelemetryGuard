#include "../src/sensor_agreement.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
}

int main() {
    const auto nominal = analyze_sensor_agreement(
        std::vector<double>{100.0, 101.0, 102.0, 103.0},
        std::vector<double>{100.2, 100.8, 102.1, 102.9},
        0.5);
    require(!nominal.degraded, "nominal redundant sensors should agree");
    require(nominal.disagreement_rate == 0.0, "nominal stream should have no disagreements");

    const auto degraded = analyze_sensor_agreement(
        std::vector<double>{10.0, 10.0, 10.0, 10.0, 10.0},
        std::vector<double>{10.0, 12.0, 10.0, 13.0, 10.0},
        0.5);
    require(degraded.degraded, "repeated disagreement should degrade sensor health");
    require(std::abs(degraded.disagreement_rate - 0.4) < 1e-9, "disagreement rate should be calculated");
    require(degraded.max_absolute_difference == 3.0, "maximum sensor delta should be tracked");

    bool mismatch_rejected = false;
    try {
        (void)analyze_sensor_agreement({1.0}, {1.0, 2.0}, 0.1);
    } catch (const std::invalid_argument&) {
        mismatch_rejected = true;
    }
    require(mismatch_rejected, "mismatched sensor streams must be rejected");

    bool non_finite_primary_rejected = false;
    try {
        (void)analyze_sensor_agreement(
            {1.0, std::numeric_limits<double>::quiet_NaN()},
            {1.0, 2.0},
            0.1);
    } catch (const std::invalid_argument&) {
        non_finite_primary_rejected = true;
    }
    require(non_finite_primary_rejected, "non-finite primary samples must be rejected");

    bool non_finite_redundant_rejected = false;
    try {
        (void)analyze_sensor_agreement(
            {1.0, 2.0},
            {1.0, std::numeric_limits<double>::infinity()},
            0.1);
    } catch (const std::invalid_argument&) {
        non_finite_redundant_rejected = true;
    }
    require(non_finite_redundant_rejected, "non-finite redundant samples must be rejected");

    bool non_finite_tolerance_rejected = false;
    try {
        (void)analyze_sensor_agreement(
            {1.0},
            {1.0},
            std::numeric_limits<double>::infinity());
    } catch (const std::invalid_argument&) {
        non_finite_tolerance_rejected = true;
    }
    require(non_finite_tolerance_rejected, "non-finite tolerance must be rejected");

    std::cout << "sensor agreement analysis passed\n";
    return 0;
}
