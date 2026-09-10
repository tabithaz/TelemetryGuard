#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

struct LimitMargin {
    double nearestWarningMargin;
    double nearestCriticalMargin;
    double warningHeadroomPercent;
};

inline LimitMargin calculateLimitMargin(
    double value,
    double warningMinimum,
    double warningMaximum,
    double criticalMinimum,
    double criticalMaximum) {
    if (!std::isfinite(value) ||
        !std::isfinite(warningMinimum) || !std::isfinite(warningMaximum) ||
        !std::isfinite(criticalMinimum) || !std::isfinite(criticalMaximum) ||
        criticalMinimum > warningMinimum || warningMinimum > warningMaximum ||
        warningMaximum > criticalMaximum) {
        throw std::invalid_argument("invalid telemetry limits");
    }

    const double warningMargin = std::min(
        std::abs(value - warningMinimum),
        std::abs(warningMaximum - value));
    const double criticalMargin = std::min(
        std::abs(value - criticalMinimum),
        std::abs(criticalMaximum - value));
    const double warningSpan = warningMaximum - warningMinimum;
    const double warningHeadroomPercent = warningSpan > 0.0
        ? std::max(0.0, std::min(100.0, (warningMargin / (warningSpan / 2.0)) * 100.0))
        : 0.0;

    return {warningMargin, criticalMargin, warningHeadroomPercent};
}
