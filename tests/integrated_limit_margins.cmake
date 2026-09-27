execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --json --fail-on never
    RESULT_VARIABLE json_result
    OUTPUT_VARIABLE json_output
    ERROR_VARIABLE json_error
)
if(NOT json_result EQUAL 0 OR NOT json_error STREQUAL "" OR
   NOT json_output MATCHES "\"nearest_warning_margin\":6750" OR
   NOT json_output MATCHES "\"nearest_critical_margin\":8750" OR
   NOT json_output MATCHES "\"warning_headroom_percent\":54" OR
   NOT json_output MATCHES "\"nearest_warning_margin\":null" OR
   NOT json_output MATCHES "\"margin_channels\":2" OR
   NOT json_output MATCHES "\"minimum_warning_headroom_percent\":10.4")
    message(FATAL_ERROR "Integrated JSON limit margins were incorrect: ${json_result}\n${json_output}\n${json_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --prometheus --fail-on never
    RESULT_VARIABLE prometheus_result
    OUTPUT_VARIABLE prometheus_output
    ERROR_VARIABLE prometheus_error
)
if(NOT prometheus_result EQUAL 0 OR NOT prometheus_error STREQUAL "" OR
   NOT prometheus_output MATCHES "telemetry_guard_margin_channels 2" OR
   NOT prometheus_output MATCHES "telemetry_guard_min_warning_headroom_percent 10.4")
    message(FATAL_ERROR "Integrated Prometheus limit margins were incorrect: ${prometheus_result}\n${prometheus_output}\n${prometheus_error}")
endif()
