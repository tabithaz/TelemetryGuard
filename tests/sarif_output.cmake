set(input_file "${CMAKE_CURRENT_BINARY_DIR}/sarif-output.csv")
file(WRITE "${input_file}"
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n"
    "Nominal Channel,5,0,10,-2,12,V,0.2,1,2\n"
    "Warning Channel,11,0,10,-2,12,V,0.2,1,2\n"
    "Missing Channel,NA,0,10,-2,12,A,0.2,1,2\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --sarif --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "SARIF output failed with ${result}: ${error}")
endif()
if(NOT error STREQUAL "")
    message(FATAL_ERROR "SARIF output wrote to stderr: ${error}")
endif()

foreach(expected
        "\"version\":\"2.1.0\""
        "\"name\":\"TelemetryGuard\""
        "\"automationDetails\":{\"id\":\"TelemetryGuard/HOLD\"}"
        "\"ruleId\":\"TG001\",\"level\":\"warning\""
        "\"ruleId\":\"TG005\",\"level\":\"error\""
        "\"channel\":\"Warning Channel\""
        "\"value\":null")
    string(FIND "${output}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "SARIF output missing: ${expected}")
    endif()
endforeach()

string(FIND "${output}" "\"channel\":\"Nominal Channel\"" nominal_position)
if(NOT nominal_position EQUAL -1)
    message(FATAL_ERROR "SARIF output should omit nominal channels")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --sarif --html
    RESULT_VARIABLE conflict_result
    OUTPUT_VARIABLE conflict_output
    ERROR_VARIABLE conflict_error
)
if(NOT conflict_result EQUAL 3)
    message(FATAL_ERROR "Conflicting output modes returned ${conflict_result}, expected 3")
endif()
string(FIND "${conflict_error}" "output modes are mutually exclusive" conflict_position)
if(conflict_position EQUAL -1)
    message(FATAL_ERROR "Conflicting output modes did not explain the error")
endif()
