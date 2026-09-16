#include "../src/dropout.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

int main() {
    const auto isolated = analyze_dropouts({100, 101, 103, 104, 106});
    if (isolated.dropout_events != 2 || isolated.total_missing_packets != 2 || isolated.longest_dropout != 1 || std::fabs(isolated.mean_missing_per_dropout - 1.0) > 1e-9 || std::fabs(isolated.missing_packet_percent - (200.0 / 7.0)) > 1e-9 || isolated.sustained_dropout) {
        return 1;
    }

    const auto sustained = analyze_dropouts({10, 11, 16, 17});
    if (sustained.dropout_events != 1 || sustained.total_missing_packets != 4 || sustained.longest_dropout != 4 || std::fabs(sustained.mean_missing_per_dropout - 4.0) > 1e-9 || std::fabs(sustained.missing_packet_percent - 50.0) > 1e-9 || !sustained.sustained_dropout) {
        return 2;
    }

    const auto complete = analyze_dropouts({20, 21, 22});
    if (complete.mean_missing_per_dropout != 0.0 || complete.missing_packet_percent != 0.0) {
        return 3;
    }

    bool rejected = false;
    try {
        (void)analyze_dropouts({5, 5, 6});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) {
        return 4;
    }

    return 0;
}
