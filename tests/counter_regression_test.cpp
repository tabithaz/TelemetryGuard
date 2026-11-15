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
    // Regression rate is measured over transitions, with an inclusive threshold.
    const auto threshold_equal = analyze_counter_regressions({10, 9, 10, 9, 10}, 0, 1, 50);
    assert(threshold_equal.regressions == 2 && threshold_equal.regression_rate_percent == 50);
    assert(threshold_equal.degraded);
    const auto threshold_above = analyze_counter_regressions({10, 9, 10, 9, 10}, 0, 1, 50.01);
    assert(!threshold_above.degraded);
    const auto all_regressions = analyze_counter_regressions({10, 9, 8}, 0, 1, 100);
    assert(all_regressions.degraded && all_regressions.regression_rate_percent == 100);

    // Only drops into the configured reset interval are exempt.
    const auto custom_resets = analyze_counter_regressions({10, 5, 10, 6, 10, 4}, 5, 6, 1);
    assert(custom_resets.regressions == 1 && custom_resets.largest_regression == 6);

    // Non-finite samples and thresholds must fail closed.
    for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity()}) {
        threw = false;
        try { analyze_counter_regressions({1, invalid}); }
        catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
        threw = false;
        try { analyze_counter_regressions({1, 2}, 0, 1, invalid); }
        catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
    }
    for (double invalid : {-0.01, 100.01}) {
        threw = false;
        try { analyze_counter_regressions({1, 2}, 0, 1, invalid); }
        catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
    }
    return 0;
}
