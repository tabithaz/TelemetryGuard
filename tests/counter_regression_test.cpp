#include "counter_regression.hpp"
#include <cassert>
#include <limits>
#include <stdexcept>

int main() {
    const auto nominal = analyze_counter_regressions({10, 11, 12, 13});
    assert(!nominal.degraded && nominal.regressions == 0);

    const auto reset = analyze_counter_regressions({98, 99, 0, 1, 2});
    assert(!reset.degraded && reset.regressions == 0);

    const auto bad = analyze_counter_regressions({10, 11, 7, 8}, 0, 1, 10);
    assert(bad.degraded && bad.regressions == 1 && bad.largest_regression == 4);

    // A zero-percent threshold must not flag healthy or expected-reset series.
    const auto single = analyze_counter_regressions({10}, 0, 1, 0);
    assert(!single.degraded && single.regressions == 0);
    const auto healthy_zero_threshold = analyze_counter_regressions({10, 11, 12}, 0, 1, 0);
    assert(!healthy_zero_threshold.degraded && healthy_zero_threshold.regression_rate_percent == 0);
    const auto reset_zero_threshold = analyze_counter_regressions({98, 99, 0, 1}, 0, 1, 0);
    assert(!reset_zero_threshold.degraded);
    const auto regression_zero_threshold = analyze_counter_regressions({10, 9}, 0, 1, 0);
    assert(regression_zero_threshold.degraded && regression_zero_threshold.regressions == 1);

    bool threw = false;
    try { analyze_counter_regressions({1, 2}, 2, 1); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    const double extreme = 1e308;
    threw = false;
    try { analyze_counter_regressions({extreme, -extreme}); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    const auto expected_reset_extreme = analyze_counter_regressions({extreme, 0.0});
    assert(expected_reset_extreme.regressions == 0);
    return 0;
}
