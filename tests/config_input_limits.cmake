set(required_file "${CMAKE_CURRENT_BINARY_DIR}/oversized-required-channels.txt")
string(REPEAT "A" 65537 oversized_line)
file(WRITE "${required_file}" "${oversized_line}\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}"
            --require-channels-file "${required_file}"
    RESULT_VARIABLE required_result
    OUTPUT_VARIABLE required_output
    ERROR_VARIABLE required_error)
if(NOT required_result EQUAL 3 OR NOT required_output STREQUAL "" OR
   NOT required_error MATCHES "required channels file line 1: row exceeds configured maximum of 65536 bytes")
    message(FATAL_ERROR "oversized required-channel row was not rejected atomically: ${required_error}")
endif()

set(policy_file "${CMAKE_CURRENT_BINARY_DIR}/oversized-policy.conf")
file(WRITE "${policy_file}" "required_channel=${oversized_line}\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --check-policy "${policy_file}"
    RESULT_VARIABLE policy_result
    OUTPUT_VARIABLE policy_output
    ERROR_VARIABLE policy_error)
if(NOT policy_result EQUAL 3 OR NOT policy_output STREQUAL "" OR
   NOT policy_error MATCHES "policy file line 1: row exceeds configured maximum of 65536 bytes")
    message(FATAL_ERROR "oversized policy row was not rejected atomically: ${policy_error}")
endif()

set(required_entries "")
set(policy_entries "")
foreach(index RANGE 1 10001)
    string(APPEND required_entries "Channel ${index}\n")
    string(APPEND policy_entries "required_channel=Channel ${index}\n")
endforeach()

file(WRITE "${required_file}" "${required_entries}")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}"
            --require-channels-file "${required_file}"
    RESULT_VARIABLE required_count_result
    OUTPUT_VARIABLE required_count_output
    ERROR_VARIABLE required_count_error)
if(NOT required_count_result EQUAL 3 OR NOT required_count_output STREQUAL "" OR
   NOT required_count_error MATCHES "line 10001: entry count exceeds maximum of 10000")
    message(FATAL_ERROR "oversized required-channel inventory was not rejected: ${required_count_error}")
endif()

file(WRITE "${policy_file}" "${policy_entries}")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --check-policy "${policy_file}"
    RESULT_VARIABLE policy_count_result
    OUTPUT_VARIABLE policy_count_output
    ERROR_VARIABLE policy_count_error)
if(NOT policy_count_result EQUAL 3 OR NOT policy_count_output STREQUAL "" OR
   NOT policy_count_error MATCHES "line 10001: entry count exceeds maximum of 10000")
    message(FATAL_ERROR "oversized policy inventory was not rejected: ${policy_count_error}")
endif()
