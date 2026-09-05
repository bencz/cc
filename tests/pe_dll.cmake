execute_process(COMMAND "${COMPILER}" --target=x86-pe -shared "${SOURCE}" -o "${OUTPUT}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "DLL generation failed: ${output}${errors}")
endif()
execute_process(COMMAND "${LOADER}" "${OUTPUT}" RESULT_VARIABLE result TIMEOUT 20)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "DLL load/call at a relocated base failed: ${result}")
endif()
