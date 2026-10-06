set(csv_file "${CMAKE_CURRENT_BINARY_DIR}/channel_count_gate.csv")
file(WRITE "${csv_file}"
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n"
    "Altitude,100,0,200,-10,210,m,0.1,1,2\n"
    "Velocity,50,0,100,-10,110,m/s,0.1,1,2\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}" --json
            --min-channels 2 --fail-on hold
    RESULT_VARIABLE passing_result
    OUTPUT_VARIABLE passing_output
    ERROR_VARIABLE passing_error)
if(NOT passing_result EQUAL 0)
    message(FATAL_ERROR "passing channel gate failed: ${passing_error}")
endif()
if(NOT passing_output MATCHES "\"minimum_channel_count\":2")
    message(FATAL_ERROR "JSON output omitted minimum channel count")
endif()
if(NOT passing_output MATCHES "\"channel_count_gate_met\":true")
    message(FATAL_ERROR "passing channel gate was not reported")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}" --json
            --min-channels 3 --fail-on hold
    RESULT_VARIABLE failing_result
    OUTPUT_VARIABLE failing_output
    ERROR_VARIABLE failing_error)
if(NOT failing_result EQUAL 2)
    message(FATAL_ERROR "incomplete snapshot did not fail with HOLD: ${failing_result} ${failing_error}")
endif()
if(NOT failing_output MATCHES "\"channel_count_gate_met\":false")
    message(FATAL_ERROR "failed channel gate was not reported")
endif()
if(NOT failing_output MATCHES "\"disposition\":\"HOLD\"")
    message(FATAL_ERROR "failed channel gate did not promote disposition")
endif()

foreach(invalid_value IN ITEMS 0 -1 nope 2.5)
    execute_process(
        COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}"
                --min-channels "${invalid_value}"
        RESULT_VARIABLE invalid_result
        ERROR_VARIABLE invalid_error)
    if(NOT invalid_result EQUAL 3)
        message(FATAL_ERROR "invalid minimum ${invalid_value} was accepted")
    endif()
    if(NOT invalid_error MATCHES "--min-channels must be a positive integer")
        message(FATAL_ERROR "invalid minimum ${invalid_value} returned the wrong error")
    endif()
endforeach()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}" --prometheus
            --min-channels 3 --fail-on never
    RESULT_VARIABLE prometheus_result
    OUTPUT_VARIABLE prometheus_output
    ERROR_VARIABLE prometheus_error)
if(NOT prometheus_result EQUAL 0)
    message(FATAL_ERROR "Prometheus channel gate failed: ${prometheus_error}")
endif()
if(NOT prometheus_output MATCHES "telemetry_guard_minimum_channel_count 3")
    message(FATAL_ERROR "Prometheus output omitted minimum channel count")
endif()
if(NOT prometheus_output MATCHES "telemetry_guard_channel_count_gate_met 0")
    message(FATAL_ERROR "Prometheus output omitted failed channel gate")
endif()
