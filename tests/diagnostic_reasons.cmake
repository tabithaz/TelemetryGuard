set(input_file "${CMAKE_CURRENT_BINARY_DIR}/diagnostic-reasons.csv")
set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n")
file(WRITE "${input_file}" "${header}"
    "Nominal,5,0,10,-2,12,V,0,1,2\n"
    "WarningHigh,11,0,10,-2,12,V,0,1,2\n"
    "WarningLow,-1,0,10,-2,12,V,0,1,2\n"
    "CriticalHigh,13,0,10,-2,12,V,0,1,2\n"
    "CriticalLow,-3,0,10,-2,12,V,0,1,2\n"
    "Aging,5,0,10,-2,12,V,1.5,1,2\n"
    "Stale,5,0,10,-2,12,V,3,1,2\n"
    "Missing,NA,0,10,-2,12,V,0,1,2\n"
    "BadAge,5,0,10,-2,12,V,-1,1,2\n"
    "BadConfig,5,10,0,-2,12,V,0,1,2\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --json --fail-on never
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT error STREQUAL "")
    message(FATAL_ERROR "Diagnostic reason report failed: ${result}\n${error}")
endif()

foreach(reason
        within_limits
        value_above_warning_maximum
        value_below_warning_minimum
        value_above_critical_maximum
        value_below_critical_minimum
        age_above_warning
        age_above_maximum
        missing_value
        invalid_age
        invalid_configuration)
    if(NOT output MATCHES "\"reason\":\"${reason}\"")
        message(FATAL_ERROR "Missing diagnostic reason ${reason}: ${output}")
    endif()
endforeach()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --junit --fail-on never
    RESULT_VARIABLE junit_result OUTPUT_VARIABLE junit_output ERROR_VARIABLE junit_error)
if(NOT junit_result EQUAL 0 OR NOT junit_error STREQUAL "" OR
   NOT junit_output MATCHES "reason=value_above_critical_maximum" OR
   NOT junit_output MATCHES "reason=missing_value")
    message(FATAL_ERROR "JUnit reasons were incorrect: ${junit_output}\n${junit_error}")
endif()
