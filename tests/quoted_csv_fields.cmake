set(header "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds")
set(valid_csv "${CMAKE_CURRENT_BINARY_DIR}/quoted-fields.csv")
file(WRITE "${valid_csv}"
    "${header}\r\n\"Cabin, Temperature\",21.5,18,25,10,35,\"deg \"\"C\"\"\",1,5,10\r\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${valid_csv}" --json --fail-on never
    RESULT_VARIABLE valid_result
    OUTPUT_VARIABLE valid_output
    ERROR_VARIABLE valid_error
)
if(NOT valid_result EQUAL 0 OR NOT valid_error STREQUAL "")
    message(FATAL_ERROR "Valid quoted CSV was rejected: ${valid_result}\n${valid_error}")
endif()
string(FIND "${valid_output}" "Cabin, Temperature" channel_position)
string(FIND "${valid_output}" "deg \\\"C\\\"" unit_position)
if(channel_position EQUAL -1 OR unit_position EQUAL -1)
    message(FATAL_ERROR "Quoted fields were not decoded in JSON output:\n${valid_output}")
endif()

set(invalid_csv "${CMAKE_CURRENT_BINARY_DIR}/unterminated-quote.csv")
file(WRITE "${invalid_csv}"
    "${header}\n\"Cabin Temperature,21.5,18,25,10,35,C,1,5,10\n")
execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${invalid_csv}" --json
    RESULT_VARIABLE invalid_result
    OUTPUT_VARIABLE invalid_output
    ERROR_VARIABLE invalid_error
)
if(NOT invalid_result EQUAL 3 OR NOT invalid_output STREQUAL "" OR
   NOT invalid_error MATCHES "CSV line 2: unterminated quoted field")
    message(FATAL_ERROR "Malformed quoted CSV was not rejected atomically: ${invalid_result}\n${invalid_output}\n${invalid_error}")
endif()
