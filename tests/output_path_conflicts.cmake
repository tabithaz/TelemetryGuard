set(input_file "${CMAKE_CURRENT_BINARY_DIR}/conflict-input.csv")
set(input_alias "${CMAKE_CURRENT_BINARY_DIR}/conflict-input-alias.csv")
set(baseline_file "${CMAKE_CURRENT_BINARY_DIR}/conflict-baseline.csv")
set(policy_file "${CMAKE_CURRENT_BINARY_DIR}/conflict.policy")
set(required_file "${CMAKE_CURRENT_BINARY_DIR}/conflict-required.txt")
set(csv_content
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\nTemperature,5,0,10,-2,12,C,0.2,1,2\n")
file(WRITE "${input_file}" "${csv_content}")
file(WRITE "${baseline_file}" "${csv_content}")
file(REMOVE "${input_alias}")
file(CREATE_LINK "${input_file}" "${input_alias}")
file(WRITE "${policy_file}" "min_health_score=90\n")
file(WRITE "${required_file}" "Temperature\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --json
            --output "${input_alias}" --fail-on never
    RESULT_VARIABLE csv_result
    OUTPUT_VARIABLE csv_output
    ERROR_VARIABLE csv_error)
file(READ "${input_file}" preserved_csv)
if(NOT csv_result EQUAL 3 OR NOT csv_output STREQUAL "" OR
   NOT csv_error MATCHES "must not replace the current CSV input" OR
   NOT preserved_csv STREQUAL csv_content)
    message(FATAL_ERROR "CSV output alias was not rejected safely: ${csv_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --policy "${policy_file}"
            --output "${policy_file}" --fail-on never
    RESULT_VARIABLE policy_result
    OUTPUT_VARIABLE policy_output
    ERROR_VARIABLE policy_error)
file(READ "${policy_file}" preserved_policy)
if(NOT policy_result EQUAL 3 OR NOT policy_output STREQUAL "" OR
   NOT policy_error MATCHES "must not replace the policy input" OR
   NOT preserved_policy STREQUAL "min_health_score=90\n")
    message(FATAL_ERROR "Policy output conflict was not rejected safely: ${policy_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}"
            --require-channels-file "${required_file}" --output "${required_file}"
            --fail-on never
    RESULT_VARIABLE required_result
    OUTPUT_VARIABLE required_output
    ERROR_VARIABLE required_error)
file(READ "${required_file}" preserved_required)
if(NOT required_result EQUAL 3 OR NOT required_output STREQUAL "" OR
   NOT required_error MATCHES "must not replace the required-channel manifest input" OR
   NOT preserved_required STREQUAL "Temperature\n")
    message(FATAL_ERROR "Required manifest conflict was not rejected safely: ${required_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --baseline "${baseline_file}"
            --output "${baseline_file}" --fail-on never
    RESULT_VARIABLE baseline_result
    OUTPUT_VARIABLE baseline_output
    ERROR_VARIABLE baseline_error)
file(READ "${baseline_file}" preserved_baseline)
if(NOT baseline_result EQUAL 3 OR NOT baseline_output STREQUAL "" OR
   NOT baseline_error MATCHES "must not replace the baseline CSV input" OR
   NOT preserved_baseline STREQUAL csv_content)
    message(FATAL_ERROR "Baseline output conflict was not rejected safely: ${baseline_error}")
endif()
