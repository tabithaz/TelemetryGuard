set(prefix "${BUILD_DIR}/install-layout-test")
file(REMOVE_RECURSE "${prefix}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "installation failed: ${install_error}\n${install_output}")
endif()

if(NOT EXISTS "${prefix}/bin/telemetry_guard")
    message(FATAL_ERROR "installed telemetry_guard executable is missing")
endif()

execute_process(
    COMMAND "${prefix}/bin/telemetry_guard" --version
    RESULT_VARIABLE version_result
    OUTPUT_VARIABLE version_output
    ERROR_VARIABLE version_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT version_result EQUAL 0 OR
   NOT version_output STREQUAL "TelemetryGuard ${EXPECTED_VERSION}")
    message(FATAL_ERROR
        "installed version check failed: ${version_error}\n${version_output}")
endif()
