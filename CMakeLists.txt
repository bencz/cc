cmake_minimum_required(VERSION 3.20)

project(cc_compiler VERSION 0.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(CC_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(WIN32)
    set(CC_HOST_SOURCE host_windows.c)
elseif(APPLE)
    set(CC_HOST_SOURCE host_macos.c)
else()
    set(CC_HOST_SOURCE host_linux.c)
endif()

add_executable(cc
    main.c
    util.c
    prepro.c
    prepro_expr.c
    lexer.c
    codegen.c
    ir.c
	semantic_ir.c
    parser.c
    expr.c
	${CC_HOST_SOURCE}
	target.c
	target_x86_pe.c
    target_zos_hlasm.c
	pe_exports.c
	backend_x86_ir.c
    backend_x86.c
    backend_hlasm.c
    zos_jcl.c
)

target_include_directories(cc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_target(cc_headers ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "$<TARGET_FILE_DIR:cc>/include"
    VERBATIM
)
add_dependencies(cc cc_headers)

if(MSVC)
    target_compile_definitions(cc PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(cc PRIVATE /W4 /WX /permissive-)
    if(CC_ENABLE_SANITIZERS)
        target_compile_options(cc PRIVATE /fsanitize=address /Zi)
        target_link_options(cc PRIVATE /INCREMENTAL:NO)
    endif()
else()
    target_compile_options(cc PRIVATE -Wall -Wextra -Wpedantic -Werror)
    if(CC_ENABLE_SANITIZERS)
        target_compile_options(cc PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(cc PRIVATE -fsanitize=address,undefined)
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
	add_executable(cc_ir_tests tests/ir_tests.c ir.c util.c)
	target_include_directories(cc_ir_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
	if(MSVC)
		target_compile_definitions(cc_ir_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
		target_compile_options(cc_ir_tests PRIVATE /W4 /WX /permissive-)
	else()
		target_compile_options(cc_ir_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
	endif()
	add_test(NAME ir.core COMMAND cc_ir_tests)

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests/hlasm")
    file(GLOB CC_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c")
	list(FILTER CC_TEST_SOURCES EXCLUDE REGEX "/ir_tests\\.c$")
    foreach(test_source IN LISTS CC_TEST_SOURCES)
        get_filename_component(test_name "${test_source}" NAME_WE)
        add_test(
            NAME "hlasm.generate.${test_name}"
            COMMAND "${CMAKE_COMMAND}"
                    "-DCOMPILER=$<TARGET_FILE:cc>"
                    "-DSOURCE=${test_source}"
                    "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/tests/hlasm/${test_name}.asm"
                    "-DJCL=${CMAKE_CURRENT_BINARY_DIR}/tests/hlasm/${test_name}.jcl"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_hlasm.cmake"
        )
    endforeach()

    function(add_cc_diagnostic_test test_name source should_succeed)
        set(extra_argument "${ARGN}")
        add_test(
            NAME "diagnostic.${test_name}"
            COMMAND "${CMAKE_COMMAND}"
                    "-DCOMPILER=$<TARGET_FILE:cc>"
                    "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/diagnostics/${source}"
                    "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/tests/hlasm/diagnostic_${test_name}.asm"
                    "-DSHOULD_SUCCEED=${should_succeed}"
                    "-DEXTRA_ARGUMENT=${extra_argument}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_diagnostic.cmake"
        )
    endfunction()
    add_cc_diagnostic_test(warning warning.c TRUE -Wno-error)
    add_cc_diagnostic_test(warning_default warning.c FALSE)
    add_cc_diagnostic_test(warning_as_error warning.c FALSE -Werror)
    add_cc_diagnostic_test(invalid_number invalid_number.c FALSE)
    add_cc_diagnostic_test(unterminated_comment unterminated_comment.c FALSE)
    add_cc_diagnostic_test(elif_after_else elif_after_else.c FALSE)
    add_cc_diagnostic_test(data_only data_only.c TRUE)

    add_test(
        NAME hlasm.translation_units
        COMMAND "${CMAKE_COMMAND}"
                "-DCOMPILER=$<TARGET_FILE:cc>"
                "-DLIBRARY_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/translation_units/library.c"
                "-DPROGRAM_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/translation_units/program.c"
                "-DLIBRARY_ASM=${CMAKE_CURRENT_BINARY_DIR}/tests/hlasm/library.asm"
                "-DPROGRAM_ASM=${CMAKE_CURRENT_BINARY_DIR}/tests/hlasm/program.asm"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/translation_units.cmake"
    )

    if(WIN32)
        set(CC_X86_RESULTS
            test_arithmetic=2704 test_arrays=15 test_basic=15 test_bitwise=765
            test_bool=0
            test_comparison=63 test_conditionals=1 test_define=220
            test_c99_regressions=0
            test_designated_initializers=0 test_enum=0 test_execution_charset=0
            test_define_nested=60 test_do_while=15 test_float=0
            test_float_arithmetic=125 test_float_binary=0 test_float_compare=0
            test_float_conversion=-4 test_function_pointers=0 test_functions=14 test_helloworld=0
            test_include_limits=1 test_include_stdarg=3 test_include_stdlib=1
            test_include_string=5 test_logical=1 test_long_external_name=0 test_loops=55
            test_nested_loops=18 test_pointers=42 test_prepro=385
            test_prepro_regressions=0 test_numeric_literals=0
            test_recursion=120 test_struct=30
            test_struct_ptr=42 test_switch=20 test_ternary=30 test_while_loop=55
            test_unsigned_integers=0 test_union=0
        )
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests/x86")
        foreach(test_result IN LISTS CC_X86_RESULTS)
            string(REPLACE "=" ";" test_fields "${test_result}")
            list(GET test_fields 0 test_name)
            list(GET test_fields 1 expected_result)
            add_test(
                NAME "x86.run.${test_name}"
                COMMAND "${CMAKE_COMMAND}"
                        "-DCOMPILER=$<TARGET_FILE:cc>"
                        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/${test_name}.c"
                        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/tests/x86/${test_name}.exe"
                        "-DEXPECTED=${expected_result}"
                        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_x86.cmake"
            )
        endforeach()

        add_test(
            NAME x86.bootstrap
            COMMAND "${CMAKE_COMMAND}"
                    "-DCOMPILER=$<TARGET_FILE:cc>"
                    "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
                    "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/bootstrap.cmake"
        )

		add_test(
			NAME x86.pe_def_imports
			COMMAND "${CMAKE_COMMAND}"
			        "-DCOMPILER=$<TARGET_FILE:cc>"
			        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/pe_imports/program.c"
			        "-DIMPORT_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}/tests/pe_imports"
			        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/tests/x86/pe_def_imports.exe"
			        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/pe_imports.cmake"
		)
    endif()
endif()
