set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds")
set(baseline_csv "${CMAKE_CURRENT_BINARY_DIR}/events_baseline.csv")
set(current_csv "${CMAKE_CURRENT_BINARY_DIR}/events_current.csv")
file(WRITE "${baseline_csv}" "${header}\nTemperature,70,-40,85,-55,100,C,1,4,5\nPressure,340,150,300,125,350,kPa,1,4,5\nVoltage,28,24,30,22,32,V,1,4,5\n")
file(WRITE "${current_csv}" "${header}\nTemperature,90,-40,85,-55,100,C,1,4,5\nPressure,250,150,300,125,350,kPa,1,4,5\nVoltage,28,24,30,22,32,V,1,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${current_csv}" --baseline "${baseline_csv}"
            --events --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
string(REGEX MATCHALL "[^\n]+" lines "${output}")
list(LENGTH lines line_count)
if(NOT result EQUAL 0 OR NOT error STREQUAL "" OR NOT line_count EQUAL 3 OR
   NOT output MATCHES "\"transition\":\"regression\"" OR
   NOT output MATCHES "\"transition\":\"recovery\"" OR
   output MATCHES "\"channel\":\"Voltage\"" OR
   NOT output MATCHES "\"type\":\"transition_summary\"" OR
   NOT output MATCHES "\"regressions\":1" OR
   NOT output MATCHES "\"recoveries\":1" OR
   NOT output MATCHES "\"unchanged\":1")
    message(FATAL_ERROR "Transition event stream was incorrect: ${result}\n${output}\n${error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --events
    RESULT_VARIABLE missing_result
    OUTPUT_VARIABLE missing_output
    ERROR_VARIABLE missing_error
)
if(NOT missing_result EQUAL 3 OR NOT missing_output STREQUAL "" OR
   NOT missing_error MATCHES "--events requires --baseline")
    message(FATAL_ERROR "Events without a baseline were not rejected: ${missing_result}\n${missing_output}\n${missing_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${current_csv}" --baseline "${baseline_csv}"
            --events --json
    RESULT_VARIABLE conflict_result
    OUTPUT_VARIABLE conflict_output
    ERROR_VARIABLE conflict_error
)
if(NOT conflict_result EQUAL 3 OR NOT conflict_output STREQUAL "" OR
   NOT conflict_error MATCHES "output modes are mutually exclusive")
    message(FATAL_ERROR "Conflicting event output was not rejected: ${conflict_result}\n${conflict_output}\n${conflict_error}")
endif()
