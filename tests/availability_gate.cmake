set(input_file "${CMAKE_CURRENT_BINARY_DIR}/availability-gate.csv")
file(WRITE "${input_file}"
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n"
    "Temperature,70,-40,85,-55,100,C,1,4,5\n"
    "Pressure,250,150,300,125,325,kPa,7,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --json
            --min-availability 75 --fail-on hold
    RESULT_VARIABLE failed_result OUTPUT_VARIABLE failed_output ERROR_VARIABLE failed_error)
if(NOT failed_result EQUAL 2 OR NOT failed_error STREQUAL "" OR
   NOT failed_output MATCHES "\"availability_percent\":50.0" OR
   NOT failed_output MATCHES "\"minimum_availability_percent\":75.0" OR
   NOT failed_output MATCHES "\"availability_gate_met\":false" OR
   NOT failed_output MATCHES "\"disposition\":\"HOLD\"")
    message(FATAL_ERROR "Availability SLO did not fail correctly: ${failed_result}\n${failed_output}\n${failed_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --prometheus
            --min-availability 50 --fail-on never
    RESULT_VARIABLE passed_result OUTPUT_VARIABLE passed_output ERROR_VARIABLE passed_error)
if(NOT passed_result EQUAL 0 OR NOT passed_error STREQUAL "" OR
   NOT passed_output MATCHES "telemetry_guard_minimum_availability_percent 50.0" OR
   NOT passed_output MATCHES "telemetry_guard_availability_gate_met 1")
    message(FATAL_ERROR "Passing availability metrics were incorrect: ${passed_result}\n${passed_output}\n${passed_error}")
endif()

foreach(invalid -1 101 nan invalid)
    execute_process(
        COMMAND "${TELEMETRY_GUARD}" --min-availability "${invalid}"
        RESULT_VARIABLE invalid_result OUTPUT_VARIABLE invalid_output ERROR_VARIABLE invalid_error)
    if(NOT invalid_result EQUAL 3 OR NOT invalid_output STREQUAL "" OR
       NOT invalid_error MATCHES "--min-availability must be a number from 0 to 100")
        message(FATAL_ERROR "Invalid availability threshold was accepted: ${invalid}\n${invalid_output}\n${invalid_error}")
    endif()
endforeach()
