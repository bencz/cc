execute_process(
    COMMAND "${COMPILER}"
            --target=x86-pe
            "-L${IMPORT_DIRECTORY}"
            -lsample
            "${SOURCE}"
            -o "${OUTPUT}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_output
    ERROR_VARIABLE compile_error
)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "PE/DEF compilation failed (${compile_result}):\n${compile_output}${compile_error}")
endif()

file(READ "${OUTPUT}" image_hex HEX)
string(TOLOWER "${image_hex}" image_hex)
foreach(expected IN ITEMS "73616d706c652e646c6c" "696d706f727465645f616464")
    string(FIND "${image_hex}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "generated PE does not contain expected import string ${expected}")
    endif()
endforeach()
