#include "../src/limit_margin.hpp"

#include <cmath>
#include <stdexcept>

namespace {
bool nearlyEqual(double left, double right) {
    return std::abs(left - right) < 1e-9;
}
}

int main() {
    const LimitMargin centered = calculateLimitMargin(50.0, 0.0, 100.0, -10.0, 110.0);
    if (!nearlyEqual(centered.nearestWarningMargin, 50.0) ||
        !nearlyEqual(centered.nearestCriticalMargin, 60.0) ||
        !nearlyEqual(centered.warningHeadroomPercent, 100.0)) {
        return 1;
    }

    const LimitMargin nearWarning = calculateLimitMargin(90.0, 0.0, 100.0, -10.0, 110.0);
    if (!nearlyEqual(nearWarning.nearestWarningMargin, 10.0) ||
        !nearlyEqual(nearWarning.warningHeadroomPercent, 20.0)) {
        return 2;
    }

    try {
        calculateLimitMargin(5.0, 10.0, 0.0, -1.0, 20.0);
        return 3;
    } catch (const std::invalid_argument&) {
        return 0;
    }
}
