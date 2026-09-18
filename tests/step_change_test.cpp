#include "step_change.hpp"

#include <cassert>
#include <stdexcept>
#include <vector>

int main() {
    const auto stable = analyze_step_changes({10.0, 10.2, 10.1, 10.3}, 1.0);
    assert(stable.step_changes == 0);
    assert(!stable.degraded);

    const auto stepped = analyze_step_changes({10.0, 10.2, 15.0, 15.1}, 2.0, 20.0);
    assert(stepped.step_changes == 1);
    assert(stepped.largest_step > 4.7);
    assert(stepped.degraded);

    const auto tolerated = analyze_step_changes({0.0, 5.0, 5.1, 5.2, 5.3}, 2.0, 30.0);
    assert(tolerated.step_changes == 1);
    assert(!tolerated.degraded);

    bool threw = false;
    try {
        analyze_step_changes({1.0}, 1.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    return 0;
}
