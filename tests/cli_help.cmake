execute_process(
    COMMAND "${TELEMETRY_GUARD}" --help
    RESULT_VARIABLE help_result
    OUTPUT_VARIABLE help_output
    ERROR_VARIABLE help_error)
if(NOT help_result EQUAL 0 OR NOT help_error STREQUAL "" OR
   NOT help_output MATCHES "Usage:" OR
   NOT help_output MATCHES "--report-csv" OR
   NOT help_output MATCHES "Exit codes:" OR
   NOT help_output MATCHES "3  Invalid input")
    message(FATAL_ERROR "Long help was incomplete: ${help_result}\n${help_output}\n${help_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" -h
    RESULT_VARIABLE short_result
    OUTPUT_VARIABLE short_output
    ERROR_VARIABLE short_error)
if(NOT short_result EQUAL 0 OR NOT short_error STREQUAL "" OR
   NOT short_output STREQUAL help_output)
    message(FATAL_ERROR "Short help differed from --help: ${short_result}\n${short_error}")
endif()

execute_process(
    COMMAND "${TELEMETRY_GUARD}" --unknown-option
    RESULT_VARIABLE invalid_result
    OUTPUT_VARIABLE invalid_output
    ERROR_VARIABLE invalid_error)
if(NOT invalid_result EQUAL 3 OR NOT invalid_output STREQUAL "" OR
   NOT invalid_error MATCHES "unknown or incomplete option" OR
   NOT invalid_error MATCHES "--help")
    message(FATAL_ERROR "Invalid option guidance was incorrect: ${invalid_result}\n${invalid_output}\n${invalid_error}")
endif()
