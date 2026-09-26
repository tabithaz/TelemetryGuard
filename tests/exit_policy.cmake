set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n")
set(monitor_csv "${CMAKE_CURRENT_BINARY_DIR}/monitor-policy.csv")
set(hold_csv "${CMAKE_CURRENT_BINARY_DIR}/hold-policy.csv")
file(WRITE "${monitor_csv}" "${header}Temperature,90,-40,85,-55,100,C,0.2,4,5\n")
file(WRITE "${hold_csv}" "${header}Battery,33,24,30,22,32,V,0.2,4,5\n")

function(run_policy input policy expected_result expected_disposition)
    execute_process(
        COMMAND "${TELEMETRY_GUARD}" --csv "${input}" --json --fail-on "${policy}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL expected_result)
        message(FATAL_ERROR "Policy ${policy} returned ${result}, expected ${expected_result}: ${error}")
    endif()
    string(FIND "${output}" "\"disposition\":\"${expected_disposition}\"" disposition_position)
    if(disposition_position EQUAL -1)
        message(FATAL_ERROR "Policy ${policy} changed or omitted the reported disposition: ${output}")
    endif()
endfunction()

run_policy("${monitor_csv}" monitor 1 MONITOR)
run_policy("${monitor_csv}" hold 0 MONITOR)
run_policy("${monitor_csv}" never 0 MONITOR)
run_policy("${hold_csv}" monitor 2 HOLD)
run_policy("${hold_csv}" hold 2 HOLD)
run_policy("${hold_csv}" never 0 HOLD)

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --fail-on sometimes
    RESULT_VARIABLE invalid_result
    ERROR_VARIABLE invalid_error
)
if(NOT invalid_result EQUAL 3 OR NOT invalid_error MATCHES "must be monitor, hold, or never")
    message(FATAL_ERROR "Invalid policy was not rejected cleanly: ${invalid_error}")
endif()
