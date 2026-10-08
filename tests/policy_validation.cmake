set(valid_policy "${CMAKE_CURRENT_BINARY_DIR}/valid-policy.conf")
file(WRITE "${valid_policy}"
    "min_health_score=90\n"
    "min_availability=99.9\n"
    "min_channels=8\n"
    "max_regressions=0\n"
    "fail_on=hold\n"
    "required_channel=Altitude\n"
    "required_channel=Battery Voltage\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --check-policy "${valid_policy}"
    RESULT_VARIABLE valid_result
    OUTPUT_VARIABLE valid_output
    ERROR_VARIABLE valid_error
)
if(NOT valid_result EQUAL 0)
    message(FATAL_ERROR "Valid policy check failed: ${valid_error}")
endif()
foreach(expected
        "\"valid\":true"
        "\"min_health_score\":90"
        "\"min_availability\":99.9"
        "\"min_channels\":8"
        "\"max_regressions\":0"
        "\"fail_on\":\"hold\""
        "\"required_channels\":[\"Altitude\",\"Battery Voltage\"]")
    string(FIND "${valid_output}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Policy JSON missing ${expected}: ${valid_output}")
    endif()
endforeach()

set(invalid_policy "${CMAKE_CURRENT_BINARY_DIR}/invalid-policy.conf")
file(WRITE "${invalid_policy}" "min_health_score=101\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --check-policy "${invalid_policy}"
    RESULT_VARIABLE invalid_result
    OUTPUT_VARIABLE invalid_output
    ERROR_VARIABLE invalid_error
)
if(NOT invalid_result EQUAL 3)
    message(FATAL_ERROR "Invalid policy should exit 3, got ${invalid_result}")
endif()
if(NOT invalid_output STREQUAL "")
    message(FATAL_ERROR "Invalid policy unexpectedly wrote stdout: ${invalid_output}")
endif()
string(FIND "${invalid_error}" "policy file line 1" line_position)
string(FIND "${invalid_error}" "min_health_score must be an integer from 0 to 100" reason_position)
if(line_position EQUAL -1 OR reason_position EQUAL -1)
    message(FATAL_ERROR "Invalid policy error lacks context: ${invalid_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --check-policy "${CMAKE_CURRENT_BINARY_DIR}/missing.conf"
    RESULT_VARIABLE missing_result
    ERROR_VARIABLE missing_error
)
if(NOT missing_result EQUAL 3)
    message(FATAL_ERROR "Missing policy should exit 3, got ${missing_result}")
endif()
string(FIND "${missing_error}" "cannot open policy file" missing_position)
if(missing_position EQUAL -1)
    message(FATAL_ERROR "Missing policy error is unclear: ${missing_error}")
endif()
