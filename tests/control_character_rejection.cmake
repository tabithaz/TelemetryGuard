set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds")
string(ASCII 27 escape)
string(ASCII 9 tab)

set(input_file "${CMAKE_CURRENT_BINARY_DIR}/control-character.csv")
file(WRITE "${input_file}" "${header}\nGuidance${escape}[31m,42,0,80,-10,100,C,0.1,1,2\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --json
    RESULT_VARIABLE channel_result
    OUTPUT_VARIABLE channel_output
    ERROR_VARIABLE channel_error)
if(NOT channel_result EQUAL 3)
    message(FATAL_ERROR "control character in channel returned ${channel_result}, expected 3")
endif()
if(NOT channel_output STREQUAL "")
    message(FATAL_ERROR "unsafe channel emitted a partial report")
endif()
if(NOT channel_error MATCHES "CSV line 2: channel name contains a control character")
    message(FATAL_ERROR "unsafe channel error was not actionable: ${channel_error}")
endif()

file(WRITE "${input_file}" "${header}\nGuidance,42,0,80,-10,100,deg${tab}C,0.1,1,2\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --json
    RESULT_VARIABLE unit_result
    OUTPUT_VARIABLE unit_output
    ERROR_VARIABLE unit_error)
if(NOT unit_result EQUAL 3 OR NOT unit_output STREQUAL "")
    message(FATAL_ERROR "control character in unit was not rejected atomically")
endif()
if(NOT unit_error MATCHES "CSV line 2: unit contains a control character")
    message(FATAL_ERROR "unsafe unit error was not actionable: ${unit_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --require-channel "Guidance${escape}[31m"
    RESULT_VARIABLE option_result
    OUTPUT_VARIABLE option_output
    ERROR_VARIABLE option_error)
if(NOT option_result EQUAL 3 OR NOT option_output STREQUAL "")
    message(FATAL_ERROR "unsafe required-channel option was not rejected atomically")
endif()
if(NOT option_error MATCHES "--require-channel cannot contain control characters")
    message(FATAL_ERROR "unsafe required-channel error was not actionable: ${option_error}")
endif()
