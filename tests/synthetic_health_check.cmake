execute_process(
    COMMAND "${TELEMETRY_GUARD}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
)

if(NOT result EQUAL 1)
    message(FATAL_ERROR "Synthetic health check returned ${result}, expected 1")
endif()

set(combined_output "${output}${error_output}")

foreach(expected_line
        "Warnings: 1"
        "Critical alerts: 1"
        "Aging readings: 1"
        "Stale readings: 1"
        "Missing readings: 1"
        "Invalid timestamps: 1"
        "Configuration errors: 0"
        "Blocking issues: 4"
        "Priority channel: Fuel Level (NO DATA)"
        "Telemetry availability: 62.5%"
        "Vehicle health score: 0/100"
        "Vehicle disposition: HOLD")
    string(FIND "${combined_output}" "${expected_line}" match_position)
    if(match_position EQUAL -1)
        message(FATAL_ERROR "Expected output not found: ${expected_line}\n${combined_output}")
    endif()
endforeach()
