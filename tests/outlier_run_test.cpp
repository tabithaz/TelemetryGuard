#include "outlier_run.hpp"

#include <stdexcept>

int main() {
    const auto healthy = analyze_outlier_runs({10.0, 10.2, 9.9, 10.1}, 9.0, 11.0, 3, 50.0);
    if (healthy.degraded || healthy.outlier_samples != 0) {
        return 1;
    }

    const auto isolated = analyze_outlier_runs({10.0, 12.0, 10.0, 8.0, 10.0}, 9.0, 11.0, 3, 50.0);
    if (isolated.degraded || isolated.longest_outlier_run != 1) {
        return 2;
    }

    const auto sustained = analyze_outlier_runs({10.0, 12.0, 12.5, 13.0, 10.0}, 9.0, 11.0, 3, 90.0);
    if (!sustained.degraded || sustained.longest_outlier_run != 3) {
        return 3;
    }

    try {
        analyze_outlier_runs({}, 0.0, 1.0);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
