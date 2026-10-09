set(csv_file "${CMAKE_CURRENT_BINARY_DIR}/input-channel-limit.csv")
file(WRITE "${csv_file}"
    "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds\n"
    "Altitude,100,0,200,-10,210,m,0,4,5\n"
    "Velocity,50,0,100,-10,110,m/s,0,4,5\n"
    "Pressure,250,150,300,125,325,kPa,0,4,5\n")

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}"
            --max-input-channels 2 --json
    RESULT_VARIABLE limited_result
    OUTPUT_VARIABLE limited_output
    ERROR_VARIABLE limited_error)
if(NOT limited_result EQUAL 3 OR NOT limited_output STREQUAL "" OR
   NOT limited_error MATCHES "CSV line 4: channel count exceeds configured maximum of 2")
    message(FATAL_ERROR "Oversized input was not rejected atomically: ${limited_result}\n${limited_output}\n${limited_error}")
endif()

foreach(invalid IN ITEMS 0 nope -1)
    execute_process(
        COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}"
                --max-input-channels "${invalid}" --json
        RESULT_VARIABLE invalid_result
        OUTPUT_VARIABLE invalid_output
        ERROR_VARIABLE invalid_error)
    if(NOT invalid_result EQUAL 3 OR NOT invalid_output STREQUAL "" OR
       NOT invalid_error MATCHES "--max-input-channels must be a positive integer")
        message(FATAL_ERROR "Invalid channel limit was accepted: ${invalid}\n${invalid_output}\n${invalid_error}")
    endif()
endforeach()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --csv "${csv_file}"
            --max-input-channels 3 --json --fail-on never
    RESULT_VARIABLE accepted_result
    OUTPUT_VARIABLE accepted_output
    ERROR_VARIABLE accepted_error)
if(NOT accepted_result EQUAL 0 OR
   NOT accepted_output MATCHES "\\\"total_readings\\\":3")
    message(FATAL_ERROR "Input at configured limit failed: ${accepted_result}\n${accepted_output}\n${accepted_error}")
endif()
