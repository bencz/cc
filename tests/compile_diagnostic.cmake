set(arguments --target=zos-hlasm -S "${SOURCE}" -o "${OUTPUT}")
if(DEFINED EXTRA_ARGUMENT AND NOT EXTRA_ARGUMENT STREQUAL "")
    list(APPEND arguments "${EXTRA_ARGUMENT}")
endif()
execute_process(
    COMMAND "${COMPILER}" ${arguments}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE standard_output
    ERROR_VARIABLE standard_error
)
if(SHOULD_SUCCEED AND NOT result EQUAL 0)
    message(FATAL_ERROR "compilation unexpectedly failed:\n${standard_output}${standard_error}")
endif()
if(NOT SHOULD_SUCCEED AND result EQUAL 0)
    message(FATAL_ERROR "compilation unexpectedly succeeded:\n${standard_output}${standard_error}")
endif()
