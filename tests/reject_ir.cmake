execute_process(COMMAND "${VERIFIER}" "${CASE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result STREQUAL "1" OR errors STREQUAL "")
    message(FATAL_ERROR "Malformed IR must produce a diagnostic, not crash: ${result}: ${output}${errors}")
endif()
