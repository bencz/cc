execute_process(
    COMMAND "${COMPILER}" --target=x86-pe "${SOURCE}" -o "${OUTPUT}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compiler_output
    ERROR_VARIABLE compiler_error
)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "x86 compilation failed (${compile_result}):\n${compiler_output}${compiler_error}")
endif()

execute_process(
    COMMAND "${OUTPUT}"
    RESULT_VARIABLE program_result
    OUTPUT_VARIABLE program_output
    ERROR_VARIABLE program_error
)
if(NOT "${program_result}" STREQUAL "${EXPECTED}")
    message(FATAL_ERROR
        "unexpected result for ${SOURCE}: expected ${EXPECTED}, got ${program_result}\n"
        "stdout:\n${program_output}\nstderr:\n${program_error}")
endif()
