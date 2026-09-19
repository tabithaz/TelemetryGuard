cmake_minimum_required(VERSION 3.16)
project(TelemetryGuard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(telemetry_guard src/main.cpp)
add_executable(limit_margin_test tests/limit_margin_test.cpp)
add_executable(rate_of_change_test tests/rate_of_change_test.cpp)
add_executable(sequence_integrity_test tests/sequence_integrity_test.cpp)
add_executable(freshness_test tests/freshness_test.cpp)
add_executable(jitter_test tests/jitter_test.cpp)
add_executable(dropout_test tests/dropout_test.cpp)
add_executable(saturation_test tests/saturation_test.cpp)
add_executable(sensor_agreement_test tests/sensor_agreement_test.cpp)
add_executable(frozen_signal_test tests/frozen_signal_test.cpp)
add_executable(sensor_drift_test tests/sensor_drift_test.cpp)
add_executable(spike_detection_test tests/spike_detection_test.cpp)
add_executable(quantization_test tests/quantization_test.cpp)
add_executable(checksum_integrity_test tests/checksum_integrity_test.cpp)
add_executable(noise_floor_test tests/noise_floor_test.cpp)
add_executable(timestamp_monotonicity_test tests/timestamp_monotonicity_test.cpp)
add_executable(gap_timing_test tests/gap_timing_test.cpp)
add_executable(outlier_run_test tests/outlier_run_test.cpp)
add_executable(bias_shift_test tests/bias_shift_test.cpp)
add_executable(variance_shift_test tests/variance_shift_test.cpp)
add_executable(range_utilization_test tests/range_utilization_test.cpp)
add_executable(slew_rate_test tests/slew_rate_test.cpp)
add_executable(deadband_test tests/deadband_test.cpp)
add_executable(step_change_test tests/step_change_test.cpp)
add_executable(timestamp_drift_test tests/timestamp_drift_test.cpp)
add_executable(oscillation_test tests/oscillation_test.cpp)
add_executable(signal_flatline_test tests/signal_flatline_test.cpp)

set(telemetry_targets
    telemetry_guard limit_margin_test rate_of_change_test sequence_integrity_test
    freshness_test jitter_test dropout_test saturation_test sensor_agreement_test
    frozen_signal_test sensor_drift_test spike_detection_test quantization_test
    checksum_integrity_test noise_floor_test timestamp_monotonicity_test gap_timing_test
    outlier_run_test bias_shift_test variance_shift_test range_utilization_test
    slew_rate_test deadband_test step_change_test timestamp_drift_test oscillation_test
    signal_flatline_test
)

foreach(target ${telemetry_targets})
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    )
endforeach()

enable_testing()
add_test(
    NAME synthetic_health_check
    COMMAND ${CMAKE_COMMAND}
        -DTELEMETRY_GUARD=$<TARGET_FILE:telemetry_guard>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/synthetic_health_check.cmake
)
add_test(NAME limit_margin_analysis COMMAND limit_margin_test)
add_test(NAME rate_of_change_analysis COMMAND rate_of_change_test)
add_test(NAME sequence_integrity_analysis COMMAND sequence_integrity_test)
add_test(NAME freshness_analysis COMMAND freshness_test)
add_test(NAME jitter_analysis COMMAND jitter_test)
add_test(NAME dropout_analysis COMMAND dropout_test)
add_test(NAME saturation_analysis COMMAND saturation_test)
add_test(NAME sensor_agreement_analysis COMMAND sensor_agreement_test)
add_test(NAME frozen_signal_analysis COMMAND frozen_signal_test)
add_test(NAME sensor_drift_analysis COMMAND sensor_drift_test)
add_test(NAME spike_detection_analysis COMMAND spike_detection_test)
add_test(NAME quantization_analysis COMMAND quantization_test)
add_test(NAME checksum_integrity_analysis COMMAND checksum_integrity_test)
add_test(NAME noise_floor_analysis COMMAND noise_floor_test)
add_test(NAME timestamp_monotonicity_analysis COMMAND timestamp_monotonicity_test)
add_test(NAME gap_timing_analysis COMMAND gap_timing_test)
add_test(NAME outlier_run_analysis COMMAND outlier_run_test)
add_test(NAME bias_shift_analysis COMMAND bias_shift_test)
add_test(NAME variance_shift_analysis COMMAND variance_shift_test)
add_test(NAME range_utilization_analysis COMMAND range_utilization_test)
add_test(NAME slew_rate_analysis COMMAND slew_rate_test)
add_test(NAME deadband_analysis COMMAND deadband_test)
add_test(NAME step_change_analysis COMMAND step_change_test)
add_test(NAME timestamp_drift_analysis COMMAND timestamp_drift_test)
add_test(NAME oscillation_analysis COMMAND oscillation_test)
add_test(NAME signal_flatline_analysis COMMAND signal_flatline_test)
