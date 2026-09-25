execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --json
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
)

if(NOT result EQUAL 2)
    message(FATAL_ERROR "JSON health check returned ${result}, expected 2\n${error_output}")
endif()

foreach(expected
        "\"channel\":\"Altitude\""
        "\"value\":null"
        "\"status\":\"NO DATA\""
        "\"availability_percent\":66.7"
        "\"health_score\":67"
        "\"disposition\":\"HOLD\"")
    string(FIND "${output}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Expected JSON fragment not found: ${expected}\n${output}")
    endif()
endforeach()

string(FIND "${output}" "TelemetryGuard - Vehicle Health Check" heading_position)
if(NOT heading_position EQUAL -1)
    message(FATAL_ERROR "JSON output included human-readable heading\n${output}")
endif()
