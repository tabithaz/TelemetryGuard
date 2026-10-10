set(input_file "${CMAKE_CURRENT_BINARY_DIR}/output-file.csv")
set(report_file "${CMAKE_CURRENT_BINARY_DIR}/telemetry-report.html")
file(WRITE "${report_file}" "previous-report")
file(WRITE "${input_file}"
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n"
    "Temperature,11,0,10,-2,12,C,0.2,1,2\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --html
            --output "${report_file}" --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0 OR NOT output STREQUAL "" OR NOT error STREQUAL "")
    message(FATAL_ERROR "Direct report output failed: ${result}\n${output}\n${error}")
endif()
file(READ "${report_file}" report)
if(NOT report MATCHES "<!doctype html>" OR
   NOT report MATCHES "Temperature" OR
   NOT report MATCHES "MONITOR")
    message(FATAL_ERROR "Direct report file was incomplete: ${report}")
endif()
file(GLOB temporary_reports "${report_file}.tmp.*")
if(temporary_reports)
    message(FATAL_ERROR "Atomic report publishing left temporary files: ${temporary_reports}")
endif()

file(WRITE "${report_file}" "preserve-me")
file(WRITE "${input_file}" "invalid header\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${input_file}" --json
            --output "${report_file}"
    RESULT_VARIABLE invalid_result
    OUTPUT_VARIABLE invalid_output
    ERROR_VARIABLE invalid_error
)
file(READ "${report_file}" preserved)
if(NOT invalid_result EQUAL 3 OR NOT invalid_output STREQUAL "" OR
   NOT invalid_error MATCHES "Input error" OR NOT preserved STREQUAL "preserve-me")
    message(FATAL_ERROR "Invalid input changed the destination: ${invalid_result}\n${invalid_output}\n${invalid_error}\n${preserved}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --json --output
    RESULT_VARIABLE missing_result
    OUTPUT_VARIABLE missing_output
    ERROR_VARIABLE missing_error
)
if(NOT missing_result EQUAL 3 OR NOT missing_output STREQUAL "" OR
   NOT missing_error MATCHES "Usage:")
    message(FATAL_ERROR "Missing output path was accepted: ${missing_result}\n${missing_output}\n${missing_error}")
endif()
