set(policy "${CMAKE_CURRENT_BINARY_DIR}/telemetry.policy")

file(WRITE "${policy}" "# release policy\nmin_health_score=90\nmin_availability=100\nmin_channels=8\nfail_on=never\nrequired_channel=Unobserved A\nrequired_channel=Unobserved B\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --json --policy "${policy}"
    RESULT_VARIABLE valid_result OUTPUT_VARIABLE valid_output ERROR_VARIABLE valid_error)
if(NOT valid_result EQUAL 0 OR
   NOT valid_output MATCHES "minimum_health_score.*90" OR
   NOT valid_output MATCHES "minimum_availability_percent.*100" OR
   NOT valid_output MATCHES "minimum_channel_count.*8" OR
   NOT valid_output MATCHES "missing_required_channels.*Unobserved A" OR
   NOT valid_output MATCHES "missing_required_channels.*Unobserved B")
    message(FATAL_ERROR "valid policy was not applied: ${valid_error}\n${valid_output}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --json --policy "${policy}"
            --min-health-score 40 --fail-on never
    RESULT_VARIABLE override_result OUTPUT_VARIABLE override_output ERROR_VARIABLE override_error)
if(NOT override_result EQUAL 0 OR
   NOT override_output MATCHES "minimum_health_score.*40")
    message(FATAL_ERROR "CLI override did not take precedence: ${override_error}\n${override_output}")
endif()

file(WRITE "${policy}" "min_channels=8\nmin_channels=9\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --policy "${policy}"
    RESULT_VARIABLE duplicate_result ERROR_VARIABLE duplicate_error)
if(NOT duplicate_result EQUAL 3 OR NOT duplicate_error MATCHES "line 2.*duplicate policy key")
    message(FATAL_ERROR "duplicate policy key was not rejected: ${duplicate_error}")
endif()

file(WRITE "${policy}" "unknown_gate=10\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --policy "${policy}"
    RESULT_VARIABLE unknown_result ERROR_VARIABLE unknown_error)
if(NOT unknown_result EQUAL 3 OR NOT unknown_error MATCHES "line 1.*unknown policy key")
    message(FATAL_ERROR "unknown policy key was not rejected: ${unknown_error}")
endif()

file(WRITE "${policy}" "min_availability=101\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --policy "${policy}"
    RESULT_VARIABLE range_result ERROR_VARIABLE range_error)
if(NOT range_result EQUAL 3 OR NOT range_error MATCHES "line 1.*min_availability")
    message(FATAL_ERROR "out-of-range policy was not rejected: ${range_error}")
endif()
