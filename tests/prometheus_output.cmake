execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --prometheus
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
)

if(NOT result EQUAL 2)
    message(FATAL_ERROR "Prometheus health check returned ${result}, expected 2\n${error_output}")
endif()

foreach(expected
        "# TYPE telemetry_guard_health_score gauge"
        "telemetry_guard_health_score 67"
        "telemetry_guard_availability_percent 66.7"
        "telemetry_guard_channels{status=\"nominal\"} 1"
        "telemetry_guard_channels{status=\"missing\"} 1"
        "telemetry_guard_disposition{disposition=\"HOLD\"} 1"
        "telemetry_guard_blocking_issues 4")
    string(FIND "${output}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Expected Prometheus metric not found: ${expected}\n${output}")
    endif()
endforeach()

string(FIND "${output}" "TelemetryGuard - Vehicle Health Check" heading_position)
if(NOT heading_position EQUAL -1)
    message(FATAL_ERROR "Prometheus output included human-readable heading\n${output}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --json --prometheus
    RESULT_VARIABLE incompatible_result
    ERROR_VARIABLE incompatible_error
)
if(NOT incompatible_result EQUAL 3)
    message(FATAL_ERROR "Incompatible output modes returned ${incompatible_result}, expected 3")
endif()
string(FIND "${incompatible_error}" "mutually exclusive" error_position)
if(error_position EQUAL -1)
    message(FATAL_ERROR "Incompatible output modes did not explain the error")
endif()
