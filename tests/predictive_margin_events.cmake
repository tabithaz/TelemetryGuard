if(NOT DEFINED TELEMETRY_GUARD)
    message(FATAL_ERROR "TELEMETRY_GUARD is required")
endif()

set(BASELINE_FILE "${CMAKE_CURRENT_BINARY_DIR}/predictive-baseline.csv")
set(CURRENT_FILE "${CMAKE_CURRENT_BINARY_DIR}/predictive-current.csv")
set(HEADER "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n")
file(WRITE "${BASELINE_FILE}" "${HEADER}Temperature,50,0,100,-10,110,C,0,4,5\nPressure,50,0,100,-10,110,kPa,0,4,5\n")
file(WRITE "${CURRENT_FILE}" "${HEADER}Temperature,80,0,100,-10,110,C,0,4,5\nPressure,55,0,100,-10,110,kPa,0,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CURRENT_FILE}" --baseline "${BASELINE_FILE}"
            --events --margin-drop-percent 25 --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Predictive event command failed (${result}): ${error}")
endif()
string(REGEX MATCHALL "[^\n]+" lines "${output}")
list(LENGTH lines line_count)
if(NOT line_count EQUAL 2)
    message(FATAL_ERROR "Expected one predictive event and one summary, got: ${output}")
endif()
foreach(pattern
        "\"type\":\"margin_regression\""
        "\"channel\":\"Temperature\""
        "\"status\":\"NOMINAL\""
        "\"previous_warning_headroom_percent\":100"
        "\"current_warning_headroom_percent\":40"
        "\"drop_percent\":60"
        "\"margin_regressions\":1"
        "\"margin_drop_threshold_percent\":25")
    if(NOT output MATCHES "${pattern}")
        message(FATAL_ERROR "Missing ${pattern} in: ${output}")
    endif()
endforeach()
if(output MATCHES "Pressure")
    message(FATAL_ERROR "Below-threshold channel should be suppressed: ${output}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CURRENT_FILE}" --baseline "${BASELINE_FILE}"
            --margin-drop-percent 25 --fail-on never
    RESULT_VARIABLE missing_events_result
    ERROR_VARIABLE missing_events_error)
if(NOT missing_events_result EQUAL 3 OR
   NOT missing_events_error MATCHES "requires --events")
    message(FATAL_ERROR "Expected --events validation, got (${missing_events_result}): ${missing_events_error}")
endif()

foreach(value 0 101 nan invalid)
    execute_process(
        COMMAND "${TELEMETRY_GUARD}" --csv "${CURRENT_FILE}" --baseline "${BASELINE_FILE}"
                --events --margin-drop-percent "${value}" --fail-on never
        RESULT_VARIABLE invalid_result
        ERROR_VARIABLE invalid_error)
    if(NOT invalid_result EQUAL 3 OR
       NOT invalid_error MATCHES "must be a number greater than 0 and at most 100")
        message(FATAL_ERROR "Expected invalid threshold rejection for ${value}, got (${invalid_result}): ${invalid_error}")
    endif()
endforeach()
