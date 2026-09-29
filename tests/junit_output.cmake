if(NOT DEFINED TELEMETRY_GUARD)
    message(FATAL_ERROR "TELEMETRY_GUARD is required")
endif()

set(CSV_FILE "${CMAKE_CURRENT_BINARY_DIR}/junit-readings.csv")
set(HEADER "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds")
file(WRITE "${CSV_FILE}" "${HEADER}\nBus & <Primary>,50,0,100,-10,110,V,0,4,5\nTemperature,105,0,100,-10,110,C,0,4,5\nPressure,50,0,100,-10,110,kPa,6,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --junit --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(NOT result EQUAL 0 OR NOT error STREQUAL "")
    message(FATAL_ERROR "JUnit report command failed (${result}): ${error}")
endif()
foreach(pattern
        "^<\\?xml version=\"1.0\" encoding=\"UTF-8\"\\?>"
        "<testsuite name=\"TelemetryGuard\" tests=\"3\" failures=\"2\" errors=\"0\" skipped=\"0\">"
        "name=\"Bus &amp; &lt;Primary&gt;\""
        "<failure type=\"WARNING\" message=\"Channel status: WARNING\">"
        "<failure type=\"STALE\" message=\"Channel status: STALE\">"
        "<property name=\"disposition\" value=\"HOLD\"/>")
    if(NOT output MATCHES "${pattern}")
        message(FATAL_ERROR "Missing JUnit XML pattern ${pattern}: ${output}")
    endif()
endforeach()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --junit --json
    RESULT_VARIABLE conflict_result
    ERROR_VARIABLE conflict_error)
if(NOT conflict_result EQUAL 3 OR NOT conflict_error MATCHES "mutually exclusive")
    message(FATAL_ERROR "Conflicting JUnit output mode was not rejected: ${conflict_error}")
endif()
