include("${CMAKE_CURRENT_LIST_DIR}/generate_ppc.cmake")
set(extra_objects)
if(NATIVE_SOURCE)
    execute_process(COMMAND "${LLVM_CLANG}" --target=powerpc-unknown-linux-gnu -O2
        -ffreestanding -fno-stack-protector -fno-pic -Wall -Wextra -Werror
        -c "${NATIVE_SOURCE}" -o "${OUTPUT}.native.o"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
    if(NOT result EQUAL 0 OR NOT errors STREQUAL "")
        message(FATAL_ERROR "Clang ABI reference compilation failed: ${output}${errors}")
    endif()
    list(APPEND extra_objects "${OUTPUT}.native.o")
endif()
execute_process(COMMAND "${LLVM_MC}" -triple=powerpc-unknown-linux-gnu -filetype=obj
    "${CMAKE_CURRENT_LIST_DIR}/ppc/linux_start.s" -o "${OUTPUT}.start.o"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR NOT errors STREQUAL "")
    message(FATAL_ERROR "PowerPC process entry assembly failed: ${output}${errors}")
endif()
execute_process(COMMAND "${LLVM_LLD}" -m elf32ppc -static -e _start
    "${OUTPUT}.start.o" "${OUTPUT}.o" ${extra_objects} -o "${OUTPUT}.elf"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR NOT errors STREQUAL "")
    message(FATAL_ERROR "PowerPC linking failed: ${output}${errors}")
endif()
if(USE_WSL)
    execute_process(COMMAND wsl.exe -d Debian -- wslpath -u "${OUTPUT}.elf"
        RESULT_VARIABLE result OUTPUT_VARIABLE executable OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Cannot translate PowerPC test executable path")
    endif()
    set(runner wsl.exe -d Debian -- "${QEMU}" "${executable}")
else()
    set(runner "${QEMU}" "${OUTPUT}.elf")
endif()
execute_process(COMMAND ${runner} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 20)
math(EXPR expected_status "${EXPECTED} & 255")
if(NOT result EQUAL expected_status)
    message(FATAL_ERROR "PowerPC program returned ${result}, expected ${expected_status}: ${output}${errors}")
endif()
