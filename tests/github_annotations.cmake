if(NOT DEFINED TELEMETRY_GUARD)
    message(FATAL_ERROR "TELEMETRY_GUARD is required")
endif()

set(CSV_FILE "${CMAKE_CURRENT_BINARY_DIR}/github-annotation-readings.csv")
set(HEADER "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds")
file(WRITE "${CSV_FILE}" "${HEADER}\nNominal,50,0,100,-10,110,V,0,4,5\n\"Bus%, Primary\",105,0,100,-10,110,V%,0,4,5\nPressure,111,0,100,-10,110,kPa,0,4,5\nNavigation,50,0,100,-10,110,%,4.5,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --github-annotations --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(NOT result EQUAL 0 OR NOT error STREQUAL "")
    message(FATAL_ERROR "GitHub annotations command failed (${result}): ${error}")
endif()

# Protect annotation field separators from CMake list expansion before counting
# newline-delimited workflow commands.
string(REPLACE ";" "\\;" escaped_output "${output}")
string(REGEX MATCHALL "[^\n]+" lines "${escaped_output}")
list(LENGTH lines line_count)
if(NOT line_count EQUAL 4)
    message(FATAL_ERROR "Expected three annotations and one summary, got ${line_count}: ${output}")
endif()

foreach(pattern
        "::warning title=TelemetryGuard%3A Bus%25%2C Primary::Channel status WARNING; value=105; unit=V%25; age_seconds=0"
        "::error title=TelemetryGuard%3A Pressure::Channel status CRITICAL"
        "::warning title=TelemetryGuard%3A Navigation::Channel status AGING"
        "::notice title=TelemetryGuard summary::Disposition HOLD, health score 62/100, blocking issues 1")
    if(NOT output MATCHES "${pattern}")
        message(FATAL_ERROR "Missing GitHub annotation pattern ${pattern}: ${output}")
    endif()
endforeach()

if(output MATCHES "TelemetryGuard%3A Nominal")
    message(FATAL_ERROR "Nominal channels should not create annotations: ${output}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --github-annotations --junit
    RESULT_VARIABLE conflict_result
    ERROR_VARIABLE conflict_error)
if(NOT conflict_result EQUAL 3 OR NOT conflict_error MATCHES "mutually exclusive")
    message(FATAL_ERROR "Conflicting annotation output mode was not rejected: ${conflict_error}")
endif()
