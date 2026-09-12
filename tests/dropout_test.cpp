#include "../src/dropout.hpp"

#include <stdexcept>
#include <vector>

int main() {
    const auto isolated = analyze_dropouts({100, 101, 103, 104, 106});
    if (isolated.dropout_events != 2 || isolated.total_missing_packets != 2 || isolated.longest_dropout != 1 || isolated.sustained_dropout) {
        return 1;
    }

    const auto sustained = analyze_dropouts({10, 11, 16, 17});
    if (sustained.dropout_events != 1 || sustained.total_missing_packets != 4 || sustained.longest_dropout != 4 || !sustained.sustained_dropout) {
        return 2;
    }

    bool rejected = false;
    try {
        (void)analyze_dropouts({5, 5, 6});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) {
        return 3;
    }

    return 0;
}
