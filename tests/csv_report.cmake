set(csv_file "${CMAKE_CURRENT_BINARY_DIR}/csv-report-readings.csv")
file(WRITE "${csv_file}"
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n"
    "=FORMULA,70,-40,85,-55,100,+unit,0.2,4,5\n"
    "Missing,NA,0,100,-1,101,%,0.8,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}" --report-csv --fail-on never
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "CSV report failed: ${result}\n${output}\n${error}")
endif()
foreach(expected IN ITEMS
        "record_type,channel,value,unit,age_seconds,status,reason"
        "channel,'=FORMULA,70,'+unit,0.2,NOMINAL,within_limits"
        "channel,Missing,,%,0.8,NO DATA,missing_value"
        "summary,,,,,,,,,,75,50,1,HOLD")
    string(FIND "${output}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "CSV report omitted ${expected}:\n${output}")
    endif()
endforeach()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}" --report-csv --json
    RESULT_VARIABLE conflict_result
    OUTPUT_VARIABLE conflict_output
    ERROR_VARIABLE conflict_error)
if(NOT conflict_result EQUAL 3 OR NOT conflict_output STREQUAL "" OR
   NOT conflict_error MATCHES "output modes are mutually exclusive")
    message(FATAL_ERROR "Conflicting CSV output mode was accepted")
endif()
