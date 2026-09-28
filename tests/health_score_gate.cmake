set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n")
set(go_csv "${CMAKE_CURRENT_BINARY_DIR}/health-gate-go.csv")
set(monitor_csv "${CMAKE_CURRENT_BINARY_DIR}/health-gate-monitor.csv")
file(WRITE "${go_csv}" "${header}Temperature,70,-40,85,-55,100,C,0.2,4,5\n")
file(WRITE "${monitor_csv}" "${header}Temperature,90,-40,85,-55,100,C,0.2,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${monitor_csv}" --json
            --min-health-score 95 --fail-on hold
    RESULT_VARIABLE failed_result
    OUTPUT_VARIABLE failed_output
    ERROR_VARIABLE failed_error
)
if(NOT failed_result EQUAL 2 OR NOT failed_error STREQUAL "" OR
   NOT failed_output MATCHES "\"health_score\":92" OR
   NOT failed_output MATCHES "\"minimum_health_score\":95" OR
   NOT failed_output MATCHES "\"health_score_gate_met\":false" OR
   NOT failed_output MATCHES "\"disposition\":\"HOLD\"")
    message(FATAL_ERROR "Health score gate did not promote MONITOR to HOLD: ${failed_result}\n${failed_output}\n${failed_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${monitor_csv}" --ndjson
            --min-health-score 92 --fail-on never
    RESULT_VARIABLE passed_result
    OUTPUT_VARIABLE passed_output
    ERROR_VARIABLE passed_error
)
if(NOT passed_result EQUAL 0 OR NOT passed_error STREQUAL "" OR
   NOT passed_output MATCHES "\"minimum_health_score\":92" OR
   NOT passed_output MATCHES "\"health_score_gate_met\":true" OR
   NOT passed_output MATCHES "\"disposition\":\"MONITOR\"")
    message(FATAL_ERROR "Passing health score gate was incorrect: ${passed_result}\n${passed_output}\n${passed_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${go_csv}" --prometheus
            --min-health-score 100
    RESULT_VARIABLE metrics_result
    OUTPUT_VARIABLE metrics_output
    ERROR_VARIABLE metrics_error
)
if(NOT metrics_result EQUAL 0 OR NOT metrics_error STREQUAL "" OR
   NOT metrics_output MATCHES "telemetry_guard_minimum_health_score 100" OR
   NOT metrics_output MATCHES "telemetry_guard_health_score_gate_met 1")
    message(FATAL_ERROR "Health score gate metrics were incorrect: ${metrics_result}\n${metrics_output}\n${metrics_error}")
endif()

foreach(invalid IN ITEMS -1 101 90.5 invalid)
    execute_process(
        COMMAND "${TELEMETRY_GUARD}" --min-health-score "${invalid}"
        RESULT_VARIABLE invalid_result
        OUTPUT_VARIABLE invalid_output
        ERROR_VARIABLE invalid_error
    )
    if(NOT invalid_result EQUAL 3 OR NOT invalid_output STREQUAL "" OR
       NOT invalid_error MATCHES "must be an integer from 0 to 100")
        message(FATAL_ERROR "Invalid health score threshold was accepted: ${invalid}\n${invalid_output}\n${invalid_error}")
    endif()
endforeach()
