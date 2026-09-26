execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv - --json
    INPUT_FILE "${CSV_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
)
if(NOT result EQUAL 2 OR NOT output MATCHES "\"disposition\":\"HOLD\"" OR
   NOT output MATCHES "\"channel\":\"Temperature\"")
    message(FATAL_ERROR "Standard-input playback failed: ${result}\n${output}\n${error_output}")
endif()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/invalid-stdin.csv" "bad,header\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv - --prometheus
    INPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/invalid-stdin.csv"
    RESULT_VARIABLE invalid_result
    OUTPUT_VARIABLE invalid_output
    ERROR_VARIABLE invalid_error
)
file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/invalid-stdin.csv")
if(NOT invalid_result EQUAL 3 OR NOT invalid_output STREQUAL "" OR
   NOT invalid_error MATCHES "Input error: invalid CSV header")
    message(FATAL_ERROR "Invalid standard input was not rejected: ${invalid_result}\n${invalid_output}\n${invalid_error}")
endif()
