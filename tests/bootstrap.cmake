set(stage2_directory "${BINARY_DIR}/tests/bootstrap")
set(stage2_compiler "${stage2_directory}/cc-stage2.exe")
set(stage3_compiler "${stage2_directory}/cc-stage3.exe")
set(stage3_program "${stage2_directory}/stage3-basic.exe")
set(stage3_assembly "${stage2_directory}/stage3-basic.asm")

file(MAKE_DIRECTORY "${stage2_directory}")
file(COPY "${SOURCE_DIR}/include" DESTINATION "${stage2_directory}")

set(compiler_sources
    main.c util.c prepro.c prepro_expr.c lexer.c codegen.c ir.c semantic_ir.c parser.c expr.c
    host_windows.c target.c target_x86_pe.c target_zos_hlasm.c
    pe_exports.c backend_x86_ir.c backend_x86.c backend_hlasm.c zos_jcl.c
)
set(source_paths)
foreach(source IN LISTS compiler_sources)
    list(APPEND source_paths "${SOURCE_DIR}/${source}")
endforeach()

execute_process(
    COMMAND "${COMPILER}" --target=x86-pe ${source_paths} -o "${stage2_compiler}"
    RESULT_VARIABLE bootstrap_result
    OUTPUT_VARIABLE bootstrap_output
    ERROR_VARIABLE bootstrap_error
)
if(NOT bootstrap_result EQUAL 0)
    message(FATAL_ERROR "stage-2 compiler generation failed (${bootstrap_result}):\n${bootstrap_output}${bootstrap_error}")
endif()

execute_process(
    COMMAND "${stage2_compiler}" --target=x86-pe ${source_paths} -o "${stage3_compiler}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_output
    ERROR_VARIABLE compile_error
)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "stage-3 compiler generation failed (${compile_result}):\n${compile_output}${compile_error}")
endif()

execute_process(
    COMMAND "${stage3_compiler}" --target=x86-pe "${SOURCE_DIR}/tests/test_basic.c" -o "${stage3_program}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_output
    ERROR_VARIABLE compile_error
)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "stage-3 x86 compilation failed (${compile_result}):\n${compile_output}${compile_error}")
endif()

execute_process(COMMAND "${stage3_program}" RESULT_VARIABLE program_result)
if(NOT program_result EQUAL 15)
    message(FATAL_ERROR "stage-3 x86 program returned ${program_result}, expected 15")
endif()

execute_process(
    COMMAND "${stage3_compiler}" --target=zos-hlasm -S "${SOURCE_DIR}/tests/test_basic.c" -o "${stage3_assembly}"
    RESULT_VARIABLE hlasm_result
    OUTPUT_VARIABLE hlasm_output
    ERROR_VARIABLE hlasm_error
)
if(NOT hlasm_result EQUAL 0)
    message(FATAL_ERROR "stage-3 HLASM generation failed (${hlasm_result}):\n${hlasm_output}${hlasm_error}")
endif()

file(READ "${stage3_assembly}" hlasm_text)
if(NOT hlasm_text MATCHES "CEEENTRY")
    message(FATAL_ERROR "stage-3 HLASM output does not contain an LE entry prolog")
endif()
