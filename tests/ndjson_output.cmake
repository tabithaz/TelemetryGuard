execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${CSV_FILE}" --ndjson --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "NDJSON output failed: ${result}\n${error}")
endif()

string(REGEX MATCHALL "[^\n]+" lines "${output}")
list(LENGTH lines line_count)
if(NOT line_count EQUAL 4)
    message(FATAL_ERROR "Expected three channel records and one summary, got ${line_count}")
endif()
list(GET lines 0 first_line)
list(GET lines 3 summary_line)
if(NOT first_line MATCHES "^\\{\"type\":\"channel\"" OR
   NOT first_line MATCHES "\"channel\":\"Altitude\"" OR
   NOT summary_line MATCHES "^\\{\"type\":\"summary\"" OR
   NOT summary_line MATCHES "\"disposition\":\"HOLD\"")
    message(FATAL_ERROR "NDJSON records are incomplete: ${output}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --json --ndjson
    RESULT_VARIABLE conflict_result
    ERROR_VARIABLE conflict_error
)
if(NOT conflict_result EQUAL 3 OR NOT conflict_error MATCHES "mutually exclusive")
    message(FATAL_ERROR "Conflicting output modes were not rejected: ${conflict_error}")
endif()
