set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds")
set(valid_row "Temperature,42,0,80,-10,100,C,0.1,1,2")
set(oversized_row "OversizedTelemetryChannelName,42,0,80,-10,100,unit-with-extra-metadata-that-keeps-growing-and-growing-and-growing-and-growing-and-growing-and-growing-and-growing-and-growing,0.1,1,2")

set(input_file "${CMAKE_CURRENT_BINARY_DIR}/row-limit.csv")
file(WRITE "${input_file}" "${header}\n${oversized_row}\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --max-row-bytes 128 --json
    RESULT_VARIABLE file_result
    OUTPUT_VARIABLE file_output
    ERROR_VARIABLE file_error)
if(NOT file_result EQUAL 3)
    message(FATAL_ERROR "oversized file row returned ${file_result}, expected 3")
endif()
if(NOT file_output STREQUAL "")
    message(FATAL_ERROR "oversized file row emitted a partial report")
endif()
if(NOT file_error MATCHES "CSV line 2: row exceeds configured maximum of 128 bytes")
    message(FATAL_ERROR "oversized file row did not report its line and limit: ${file_error}")
endif()

file(WRITE "${input_file}" "${header}\n${valid_row}\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv - --max-row-bytes 128 --json --fail-on never
    INPUT_FILE "${input_file}"
    RESULT_VARIABLE stdin_result
    OUTPUT_VARIABLE stdin_output
    ERROR_VARIABLE stdin_error)
if(NOT stdin_result EQUAL 0)
    message(FATAL_ERROR "bounded stdin input returned ${stdin_result}: ${stdin_error}")
endif()
if(NOT stdin_output MATCHES "\"channel\":\"Temperature\"")
    message(FATAL_ERROR "bounded stdin input did not emit the expected report")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --max-row-bytes 0
    RESULT_VARIABLE option_result
    ERROR_VARIABLE option_error)
if(NOT option_result EQUAL 3 OR
   NOT option_error MATCHES "--max-row-bytes must be a positive integer")
    message(FATAL_ERROR "invalid row limit was not rejected correctly: ${option_error}")
endif()
