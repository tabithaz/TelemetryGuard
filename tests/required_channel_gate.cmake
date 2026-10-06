set(csv_file "${CMAKE_CURRENT_BINARY_DIR}/required_channel_gate.csv")
file(WRITE "${csv_file}"
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n"
    "Altitude,100,0,200,-10,210,m,0.1,1,2\n"
    "Velocity,50,0,100,-10,110,m/s,0.1,1,2\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}" --json
            --require-channel Altitude --require-channel Velocity --fail-on hold
    RESULT_VARIABLE passing_result
    OUTPUT_VARIABLE passing_output
    ERROR_VARIABLE passing_error)
if(NOT passing_result EQUAL 0)
    message(FATAL_ERROR "present required channels failed: ${passing_error}")
endif()
if(NOT passing_output MATCHES "\"required_channels_gate_met\":true")
    message(FATAL_ERROR "passing required-channel gate was not reported")
endif()
if(NOT passing_output MATCHES "\"missing_required_channels\":\\[\\]")
    message(FATAL_ERROR "passing required-channel gate reported missing channels")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}" --json
            --min-channels 2 --require-channel Pressure --fail-on hold
    RESULT_VARIABLE failing_result
    OUTPUT_VARIABLE failing_output
    ERROR_VARIABLE failing_error)
if(NOT failing_result EQUAL 2)
    message(FATAL_ERROR "missing required channel did not HOLD: ${failing_result} ${failing_error}")
endif()
if(NOT failing_output MATCHES "\"channel_count_gate_met\":true")
    message(FATAL_ERROR "count gate should pass when identity gate fails")
endif()
if(NOT failing_output MATCHES "\"required_channels_gate_met\":false")
    message(FATAL_ERROR "failed required-channel gate was not reported")
endif()
if(NOT failing_output MATCHES "\"missing_required_channels\":\\[\"Pressure\"\\]")
    message(FATAL_ERROR "missing required channel identity was not reported")
endif()
if(NOT failing_output MATCHES "\"disposition\":\"HOLD\"")
    message(FATAL_ERROR "missing required channel did not promote disposition")
endif()

foreach(invalid_value IN ITEMS "" " " " Altitude" "Altitude ")
    execute_process(
        COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}"
                --require-channel "${invalid_value}"
        RESULT_VARIABLE invalid_result
        ERROR_VARIABLE invalid_error)
    if(NOT invalid_result EQUAL 3)
        message(FATAL_ERROR "invalid required channel was accepted")
    endif()
endforeach()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}"
            --require-channel Altitude --require-channel Altitude
    RESULT_VARIABLE duplicate_result
    ERROR_VARIABLE duplicate_error)
if(NOT duplicate_result EQUAL 3 OR
   NOT duplicate_error MATCHES "duplicate --require-channel")
    message(FATAL_ERROR "duplicate required channel was accepted")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}" --prometheus
            --require-channel Pressure --fail-on never
    RESULT_VARIABLE prometheus_result
    OUTPUT_VARIABLE prometheus_output
    ERROR_VARIABLE prometheus_error)
if(NOT prometheus_result EQUAL 0)
    message(FATAL_ERROR "Prometheus required-channel gate failed: ${prometheus_error}")
endif()
if(NOT prometheus_output MATCHES "telemetry_guard_missing_required_channels 1")
    message(FATAL_ERROR "Prometheus output omitted missing required channel count")
endif()
if(NOT prometheus_output MATCHES "telemetry_guard_required_channels_gate_met 0")
    message(FATAL_ERROR "Prometheus output omitted failed required-channel gate")
endif()
