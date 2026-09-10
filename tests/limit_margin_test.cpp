#include "../src/limit_margin.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

int main() {
    const LimitMargin centered = calculateLimitMargin(50.0, 0.0, 100.0, -10.0, 110.0);
    assert(std::abs(centered.nearestWarningMargin - 50.0) < 1e-9);
    assert(std::abs(centered.nearestCriticalMargin - 60.0) < 1e-9);
    assert(std::abs(centered.warningHeadroomPercent - 100.0) < 1e-9);

    const LimitMargin nearWarning = calculateLimitMargin(90.0, 0.0, 100.0, -10.0, 110.0);
    assert(std::abs(nearWarning.nearestWarningMargin - 10.0) < 1e-9);
    assert(std::abs(nearWarning.warningHeadroomPercent - 20.0) < 1e-9);

    bool threw = false;
    try {
        calculateLimitMargin(5.0, 10.0, 0.0, -1.0, 20.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    return 0;
}
