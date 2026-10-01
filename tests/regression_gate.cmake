set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n")
set(baseline "${CMAKE_CURRENT_BINARY_DIR}/regression-gate-baseline.csv")
set(current "${CMAKE_CURRENT_BINARY_DIR}/regression-gate-current.csv")
file(WRITE "${baseline}" "${header}Temperature,5,0,10,-2,12,C,0.2,1,2\nPressure,5,0,10,-2,12,kPa,0.2,1,2\n")
file(WRITE "${current}" "${header}Temperature,11,0,10,-2,12,C,0.2,1,2\nPressure,5,0,10,-2,12,kPa,0.2,1,2\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${current}" --baseline "${baseline}"
            --json --max-regressions 0 --fail-on hold
    RESULT_VARIABLE failed_result OUTPUT_VARIABLE failed_output ERROR_VARIABLE failed_error)
if(NOT failed_result EQUAL 2 OR NOT failed_error STREQUAL "" OR
   NOT failed_output MATCHES "\"regressions\":1" OR
   NOT failed_output MATCHES "\"maximum_regressions\":0" OR
   NOT failed_output MATCHES "\"regression_gate_met\":false" OR
   NOT failed_output MATCHES "\"disposition\":\"HOLD\"")
    message(FATAL_ERROR "Regression gate did not fail correctly: ${failed_result}\n${failed_output}\n${failed_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${current}" --baseline "${baseline}"
            --prometheus --max-regressions 1 --fail-on never
    RESULT_VARIABLE passed_result OUTPUT_VARIABLE passed_output ERROR_VARIABLE passed_error)
if(NOT passed_result EQUAL 0 OR NOT passed_error STREQUAL "" OR
   NOT passed_output MATCHES "telemetry_guard_maximum_regressions 1" OR
   NOT passed_output MATCHES "telemetry_guard_regression_gate_met 1")
    message(FATAL_ERROR "Passing regression metrics were incorrect: ${passed_result}\n${passed_output}\n${passed_error}")
endif()

foreach(arguments "--max-regressions;-1" "--max-regressions;one" "--max-regressions;0")
    set(command "${TELEMETRY_GUARD}")
    if(arguments STREQUAL "--max-regressions;0")
        execute_process(COMMAND "${TELEMETRY_GUARD}" --max-regressions 0
            RESULT_VARIABLE invalid_result OUTPUT_VARIABLE invalid_output ERROR_VARIABLE invalid_error)
        set(expected "requires --baseline")
    else()
        execute_process(COMMAND "${TELEMETRY_GUARD}" --baseline "${baseline}" ${arguments}
            RESULT_VARIABLE invalid_result OUTPUT_VARIABLE invalid_output ERROR_VARIABLE invalid_error)
        set(expected "must be a non-negative integer")
    endif()
    if(NOT invalid_result EQUAL 3 OR NOT invalid_output STREQUAL "" OR
       NOT invalid_error MATCHES "${expected}")
        message(FATAL_ERROR "Invalid regression budget was accepted: ${arguments}\n${invalid_error}")
    endif()
endforeach()
