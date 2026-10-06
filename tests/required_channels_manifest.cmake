set(manifest "${CMAKE_CURRENT_BINARY_DIR}/required-channels.txt")
file(WRITE "${manifest}" "# mission-critical inventory\n\nAltitude\nBattery Voltage\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --json
            --require-channels-file "${manifest}" --fail-on never
    RESULT_VARIABLE pass_result OUTPUT_VARIABLE pass_output ERROR_VARIABLE pass_error)
if(NOT pass_result EQUAL 0 OR NOT pass_output MATCHES "required_channels_gate_met.*true")
    message(FATAL_ERROR "valid manifest failed: ${pass_error}\n${pass_output}")
endif()

file(WRITE "${manifest}" "Altitude\nUnobserved Critical Signal\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --json
            --require-channels-file "${manifest}" --fail-on never
    RESULT_VARIABLE missing_result OUTPUT_VARIABLE missing_output ERROR_VARIABLE missing_error)
if(NOT missing_result EQUAL 0 OR
   NOT missing_output MATCHES "required_channels_gate_met.*false" OR
   NOT missing_output MATCHES "Unobserved Critical Signal" OR
   NOT missing_output MATCHES "disposition.*HOLD")
    message(FATAL_ERROR "missing manifest channel was not enforced: ${missing_error}\n${missing_output}")
endif()

file(WRITE "${manifest}" "Altitude\nAltitude\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}"
            --require-channels-file "${manifest}"
    RESULT_VARIABLE duplicate_result ERROR_VARIABLE duplicate_error)
if(NOT duplicate_result EQUAL 3 OR NOT duplicate_error MATCHES "duplicate required channel: Altitude")
    message(FATAL_ERROR "duplicate manifest entry was not rejected: ${duplicate_error}")
endif()

file(WRITE "${manifest}" "  Altitude\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}"
            --require-channels-file "${manifest}"
    RESULT_VARIABLE whitespace_result ERROR_VARIABLE whitespace_error)
if(NOT whitespace_result EQUAL 3 OR NOT whitespace_error MATCHES "line 1")
    message(FATAL_ERROR "invalid manifest whitespace was not rejected: ${whitespace_error}")
endif()
