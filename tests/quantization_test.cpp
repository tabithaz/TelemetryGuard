#include "quantization.hpp"

int main() {
    const auto good = analyze_quantization({0.0, 0.5, 1.0, 1.5}, 0.5);
    if (good.degraded) return 1;

    const auto repeated = analyze_quantization({2.0, 2.0, 2.0, 2.0, 2.0, 3.0}, 1.0);
    if (!repeated.degraded) return 2;

    const auto off_grid = analyze_quantization({0.0, 0.7, 1.4, 2.1}, 0.5);
    if (!off_grid.degraded) return 3;

    try {
        analyze_quantization({1.0}, 0.5);
        return 4;
    } catch (const std::invalid_argument&) {
    }

    return 0;
}
