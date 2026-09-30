set(input_file "${CMAKE_CURRENT_BINARY_DIR}/html-output.csv")
file(WRITE "${input_file}"
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n"
    "Bus & <Primary>,11,0,10,-2,12,V & A,0.2,1,2\n"
    "Fuel Level,NA,0,100,-1,101,%,0.2,1,2\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --html --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "HTML output failed with ${result}: ${error}")
endif()
if(NOT error STREQUAL "")
    message(FATAL_ERROR "HTML output wrote to stderr: ${error}")
endif()

foreach(expected
        "<!doctype html>"
        "<title>TelemetryGuard Health Report</title>"
        "Bus &amp; &lt;Primary&gt;"
        "V &amp; A"
        "class=\"status warning\">WARNING"
        "class=\"status critical\">NO DATA"
        "class=\"hero-status critical\">HOLD"
        "Health score"
        "67/100"
        "Blocking issues")
    string(FIND "${output}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "HTML output missing: ${expected}")
    endif()
endforeach()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --html --junit
    RESULT_VARIABLE conflict_result
    OUTPUT_VARIABLE conflict_output
    ERROR_VARIABLE conflict_error
)
if(NOT conflict_result EQUAL 3)
    message(FATAL_ERROR "Conflicting output modes returned ${conflict_result}, expected 3")
endif()
if(NOT conflict_output STREQUAL "")
    message(FATAL_ERROR "Conflicting output modes unexpectedly wrote a report")
endif()
string(FIND "${conflict_error}" "output modes are mutually exclusive" conflict_position)
if(conflict_position EQUAL -1)
    message(FATAL_ERROR "Conflicting output modes did not explain the error")
endif()
