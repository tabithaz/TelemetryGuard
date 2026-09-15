#include "../src/noise_floor.hpp"

#include <stdexcept>

int main() {
    const auto healthy = analyze_noise_floor({1.0, 1.2, 0.9, 1.1}, 0.01, 0.5);
    if (healthy.degraded) return 1;

    const auto noisy = analyze_noise_floor({0.0, 2.0, -2.0, 2.0}, 0.01, 1.0);
    if (!noisy.degraded || noisy.rms_delta <= 1.0) return 2;

    const auto flat = analyze_noise_floor({5.0, 5.0, 5.0, 5.0, 5.0}, 0.01, 1.0, 75.0);
    if (!flat.degraded || flat.quiet_percent != 100.0) return 3;

    try {
        analyze_noise_floor({1.0}, 0.01, 1.0);
        return 4;
    } catch (const std::invalid_argument&) {
    }
    return 0;
}
