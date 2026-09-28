set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds")
set(baseline_csv "${CMAKE_CURRENT_BINARY_DIR}/baseline.csv")
set(current_csv "${CMAKE_CURRENT_BINARY_DIR}/current.csv")
file(WRITE "${baseline_csv}" "${header}\nTemperature,70,-40,85,-55,100,C,1,4,5\nPressure,340,150,300,125,350,kPa,1,4,5\nVoltage,28,24,30,22,32,V,1,4,5\n")
file(WRITE "${current_csv}" "${header}\nTemperature,90,-40,85,-55,100,C,1,4,5\nPressure,250,150,300,125,350,kPa,1,4,5\nVoltage,28,24,30,22,32,V,1,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${current_csv}" --baseline "${baseline_csv}"
            --json --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0 OR NOT error STREQUAL "" OR
   NOT output MATCHES "\"comparison_enabled\":true" OR
   NOT output MATCHES "\"regressions\":1" OR
   NOT output MATCHES "\"recoveries\":1" OR
   NOT output MATCHES "\"unchanged\":1" OR
   NOT output MATCHES "\"previous_status\":\"NOMINAL\"")
    message(FATAL_ERROR "Baseline comparison output was incorrect: ${result}\n${output}\n${error}")
endif()

file(WRITE "${baseline_csv}" "${header}\nDifferent,70,-40,85,-55,100,C,1,4,5\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${current_csv}" --baseline "${baseline_csv}"
    RESULT_VARIABLE mismatch_result
    OUTPUT_VARIABLE mismatch_output
    ERROR_VARIABLE mismatch_error
)
if(NOT mismatch_result EQUAL 3 OR NOT mismatch_output STREQUAL "" OR
   NOT mismatch_error MATCHES "baseline channels do not match current channels")
    message(FATAL_ERROR "Mismatched baseline was not rejected atomically: ${mismatch_result}\n${mismatch_output}\n${mismatch_error}")
endif()

file(WRITE "${baseline_csv}" "${header}\nTemperature,70,-40,85,-55,100,F,1,4,5\nPressure,340,150,300,125,350,kPa,1,4,5\nVoltage,28,24,30,22,32,V,1,4,5\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${current_csv}" --baseline "${baseline_csv}"
            --json --fail-on never
    RESULT_VARIABLE configuration_result
    OUTPUT_VARIABLE configuration_output
    ERROR_VARIABLE configuration_error
)
if(NOT configuration_result EQUAL 3 OR NOT configuration_output STREQUAL "" OR
   NOT configuration_error MATCHES "baseline configuration differs for channel: Temperature")
    message(FATAL_ERROR "Baseline configuration drift was not rejected atomically: ${configuration_result}\n${configuration_output}\n${configuration_error}")
endif()

file(WRITE "${baseline_csv}" "${header}\nTemperature,70,-40,85,-55,100,C,1,4,5\nPressure,340,150,300,125,350,kPa,1,4,5\nVoltage,28,24,30,22,32,V,1,4,6\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${current_csv}" --baseline "${baseline_csv}"
            --events --fail-on never
    RESULT_VARIABLE threshold_result
    OUTPUT_VARIABLE threshold_output
    ERROR_VARIABLE threshold_error
)
if(NOT threshold_result EQUAL 3 OR NOT threshold_output STREQUAL "" OR
   NOT threshold_error MATCHES "baseline configuration differs for channel: Voltage")
    message(FATAL_ERROR "Baseline age-threshold drift was not rejected: ${threshold_result}\n${threshold_output}\n${threshold_error}")
endif()
