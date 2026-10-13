set(input_file "${CMAKE_CURRENT_BINARY_DIR}/validation-input.csv")
set(baseline_file "${CMAKE_CURRENT_BINARY_DIR}/validation-baseline.csv")
set(csv_content
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\nTemperature,15,0,10,-2,12,C,0.2,1,2\nPressure,200,150,300,125,325,kPa,0.2,1,2\n")
file(WRITE "${input_file}" "${csv_content}")
file(WRITE "${baseline_file}" "${csv_content}")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --validate-input --csv "${input_file}"
            --baseline "${baseline_file}" --require-channel Temperature
    RESULT_VARIABLE valid_result
    OUTPUT_VARIABLE valid_output
    ERROR_VARIABLE valid_error)
if(NOT valid_result EQUAL 0 OR NOT valid_error STREQUAL "" OR
   NOT valid_output STREQUAL
       "{\"schema_version\":1,\"valid\":true,\"channels\":2,\"baseline_channels\":2,\"required_channels\":1}\n")
    message(FATAL_ERROR "Valid input preflight failed: ${valid_result}\n${valid_output}\n${valid_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --validate-input --csv "${input_file}"
            --require-channel Navigation
    RESULT_VARIABLE missing_result
    OUTPUT_VARIABLE missing_output
    ERROR_VARIABLE missing_error)
if(NOT missing_result EQUAL 3 OR NOT missing_output STREQUAL "" OR
   NOT missing_error MATCHES "missing required channel: Navigation")
    message(FATAL_ERROR "Missing required channel passed preflight: ${missing_result}\n${missing_output}\n${missing_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --validate-input
    RESULT_VARIABLE absent_result
    OUTPUT_VARIABLE absent_output
    ERROR_VARIABLE absent_error)
if(NOT absent_result EQUAL 3 OR NOT absent_output STREQUAL "" OR
   NOT absent_error MATCHES "--validate-input requires --csv")
    message(FATAL_ERROR "Input-less preflight was accepted: ${absent_result}\n${absent_output}\n${absent_error}")
endif()
