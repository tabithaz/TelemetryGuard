#include "../src/freshness.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
}

int main() {
    const auto fresh = analyze_freshness({0.1, 0.3, 0.5}, 1.0);
    require(std::string(fresh.status) == "FRESH", "fresh telemetry should remain FRESH");
    require(std::abs(fresh.maximum_age_seconds - 0.5) < 1e-9, "maximum age mismatch");

    const auto stale = analyze_freshness({0.2, 1.4, 0.8, 0.6, 0.4}, 1.0, 40.0);
    require(std::string(stale.status) == "STALE", "single stale reading should be STALE");
    require(std::abs(stale.stale_percentage - 20.0) < 1e-9, "stale percentage mismatch");

    const auto critical = analyze_freshness({2.0, 1.5, 0.2, 0.1}, 1.0, 25.0);
    require(std::string(critical.status) == "CRITICAL", "high stale share should be CRITICAL");
    require(std::abs(critical.average_age_seconds - 0.95) < 1e-9, "average age mismatch");

    bool rejected_negative_age = false;
    try {
        (void)analyze_freshness({0.2, -0.1}, 1.0);
    } catch (const std::invalid_argument&) {
        rejected_negative_age = true;
    }
    require(rejected_negative_age, "negative ages must be rejected");

    std::cout << "freshness diagnostics passed\n";
    return 0;
}
