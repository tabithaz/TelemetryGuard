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

target_compile_options(telemetry_guard PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
target_compile_options(limit_margin_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
target_compile_options(rate_of_change_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
target_compile_options(sequence_integrity_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
target_compile_options(freshness_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
target_compile_options(jitter_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

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
