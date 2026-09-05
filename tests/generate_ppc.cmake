execute_process(COMMAND "${COMPILER}" "--target=${TARGET}" -S "${SOURCE}" -o "${OUTPUT}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "PowerPC compilation failed: ${output}${errors}")
endif()
file(READ "${OUTPUT}" assembly)
if(assembly MATCHES "TODO|FIXME|not implemented|eax|ecx|ebp|esp")
    message(FATAL_ERROR "PowerPC output contains an incomplete or foreign lowering")
endif()
if("${TARGET}" STREQUAL "ppc32-linux" AND LLVM_MC)
    execute_process(COMMAND "${LLVM_MC}" -triple=powerpc-unknown-linux-gnu -filetype=obj
        "${OUTPUT}" -o "${OUTPUT}.o"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
    if(NOT result EQUAL 0 OR NOT errors STREQUAL "")
        message(FATAL_ERROR "PowerPC assembly failed: ${output}${errors}")
    endif()
    execute_process(COMMAND "${LLVM_READOBJ}" --file-headers "${OUTPUT}.o"
        RESULT_VARIABLE result OUTPUT_VARIABLE header ERROR_VARIABLE errors)
    if(NOT result EQUAL 0 OR NOT header MATCHES "elf32-powerpc" OR NOT header MATCHES "BigEndian")
        message(FATAL_ERROR "Invalid PowerPC object: ${header}${errors}")
    endif()
elseif("${TARGET}" STREQUAL "ppc32-aix")
    if(NOT assembly MATCHES "csect" OR NOT assembly MATCHES "\\[DS\\]" OR
       NOT assembly MATCHES "TOC\\[TC0\\]")
        message(FATAL_ERROR "Missing AIX descriptor/TOC linkage")
    endif()
endif()
