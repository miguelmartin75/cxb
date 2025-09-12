cmake_minimum_required(VERSION 3.19)

project(cxb LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

option(CXB_BUILD_TOOLS "build tools?" OFF)
option(CXB_BUILD_TESTS "enable tests?" OFF)
option(CXB_BUILD_EXAMPLES "enable examples?" OFF)
option(CXB_BUILD_FUZZERS "build fuzzers?" OFF)
option(CXB_BUILD_C_API_TESTS "Build C API compatibility tests" ON)

set(CXB_SRCS "cxb/cxb.cpp")

# Add compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(-ftime-trace -Wall -Wextra)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra)
endif()

function(add_exe name sources)
    add_executable(${name} ${sources} ${CXB_SRCS})
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 23)
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
        target_compile_options(${name} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${name} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()
endfunction()

function(add_test_exe name test_source fsanitize)
    add_executable(${name} ${test_source} ${CXB_SRCS})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARGN})
    if(${fsanitize} AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
        target_compile_options(${name} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${name} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()
endfunction()

function(add_fuzz_exe name sources)
    add_executable(${name} ${sources} ${CXB_SRCS})
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 23)
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(${name} PRIVATE
        -fsanitize=fuzzer,address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(${name} PRIVATE
        -fsanitize=fuzzer,address,undefined
    )
endfunction()

if(CXB_BUILD_EXAMPLES)
    set(INTERPRETER_SRCS examples/interpreter.cpp examples/parser.cpp examples/memfile.cpp)
    add_exe(interpreter "${INTERPRETER_SRCS}")
endif()

if(CXB_BUILD_TOOLS)
    add_exe(extract_capi scripts/extract_capi.cpp)
endif()

if(CXB_BUILD_TESTS)
    add_subdirectory(deps/Catch2)

    enable_testing()

    add_test_exe(test_array tests/test_array.cpp 1)
    add_test_exe(test_string tests/test_string.cpp 1)
    add_test_exe(test_arena tests/test_arena.cpp 1)
    add_test_exe(test_hm tests/test_hm.cpp 1)
    add_test_exe(test_algos tests/test_algos.cpp 1)
    add_test_exe(test_format tests/test_format.cpp 1)

    add_test_exe(bench_string tests/benchs/bench_string.cpp 1)
    add_test_exe(bench_string_header tests/benchs/bench_string_header.cpp 1)
    add_test_exe(bench_std_headers tests/benchs/bench_std_headers.cpp 0)
    add_test_exe(bench_hm tests/benchs/bench_hm.cpp 1)
    add_test_exe(bench_algos tests/benchs/bench_algos.cpp 0)

    add_test(NAME test_array COMMAND test_array)
    add_test(NAME test_string COMMAND test_string)
    add_test(NAME test_arena COMMAND test_arena)
    add_test(NAME test_hm COMMAND test_hm)
    add_test(NAME test_algos COMMAND test_algos)

    # if(CXB_BUILD_C_API_TESTS)
    if(0)  # TODO
        # Pure C test
        add_executable(test_c_api_c tests/test_c_api_c.c tests/c_api_test.cpp ${CXB_SRCS})
        target_include_directories(test_c_api_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
        # target_link_libraries(test_c_api_c PRIVATE cxb)

        # C++ test that uses Catch2
        add_executable(test_c_api_cpp tests/test_c_api_cpp.cpp tests/c_api_test.cpp ${CXB_SRCS})
        target_include_directories(test_c_api_cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_c_api_cpp PRIVATE Catch2::Catch2WithMain)

        add_test(NAME test_c_api_c COMMAND test_c_api_c)
        add_test(NAME test_c_api_cpp COMMAND test_c_api_cpp)
    endif()
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fsanitize=fuzzer" CXB_HAS_FUZZER)
if(CXB_BUILD_FUZZERS AND CXB_HAS_FUZZER)
    add_fuzz_exe(fuzz_hashmap tests/fuzz/fuzz_hashmap.cpp)
    add_fuzz_exe(fuzz_string tests/fuzz/fuzz_string.cpp)
    add_fuzz_exe(fuzz_format_float tests/fuzz/fuzz_format_float.cpp)

    add_test(NAME fuzz_hashmap COMMAND fuzz_hashmap -max_total_time=60)
    add_test(NAME fuzz_string COMMAND fuzz_string -max_total_time=60)
    add_test(NAME fuzz_format_float COMMAND fuzz_format_float -max_total_time=60)
elseif(CXB_BUILD_FUZZERS)
    message(WARNING "CXB_BUILD_FUZZERS is ON, but compiler does not support -fsanitize=fuzzer (CXB_HAS_FUZZER=${CXB_HAS_FUZZER})")
endif()
