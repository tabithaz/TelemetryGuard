execute_process(COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error_output)
if(NOT result EQUAL 2 OR NOT output MATCHES "Warnings: 1" OR
   NOT output MATCHES "Missing readings: 1" OR
   NOT output MATCHES "Vehicle disposition: HOLD")
    message(FATAL_ERROR "CSV playback failed: ${result}\n${output}\n${error_output}")
endif()
execute_process(COMMAND "${TELEMETRY_GUARD}" --csv "${CMAKE_CURRENT_BINARY_DIR}/missing.csv"
    RESULT_VARIABLE bad_result OUTPUT_VARIABLE bad_output ERROR_VARIABLE bad_error)
if(NOT bad_result EQUAL 3 OR NOT bad_error MATCHES "Input error:")
    message(FATAL_ERROR "Missing CSV was not rejected: ${bad_result}\n${bad_error}")
endif()
