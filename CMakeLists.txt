cmake_minimum_required(VERSION 3.19)

project(cxb LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

option(CXB_BUILD_TOOLS "build tools?" OFF)
option(CXB_BUILD_TESTS "enable tests?" OFF)
option(CXB_BUILD_EXAMPLES "enable examples?" OFF)
option(CXB_BUILD_C_API_TESTS "Build C API compatibility tests" ON)

set(CXB_SRCS "cxb/cxb.cpp")

# Add compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(-ftime-trace -Wall -Wextra)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra)
endif()

function(add_test_exe test_name test_source fsanitize)
    add_executable(${test_name} ${test_source} ${CXB_SRCS})
    target_include_directories(${test_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${test_name} PRIVATE Catch2::Catch2WithMain ${ARGN})
    if(${fsanitize} AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
        target_compile_options(${test_name} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${test_name} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()
endfunction()

function(add_exe name sources)
    add_executable(${name} ${sources} ${CXB_SRCS} deps/fmt/src/format.cc)
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} deps/fmt/include)
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

if(CXB_BUILD_EXAMPLES)
    set(INTERPRETER_SRCS examples/interpreter.cpp examples/parser.cpp examples/memfile.cpp)
    add_exe(interpreter "${INTERPRETER_SRCS}")
endif()

if(CXB_BUILD_EXAMPLES)
    add_exe(extract_capi tools/extract_capi.cpp)
endif()

if(CXB_BUILD_TESTS)
    add_subdirectory(deps/Catch2)

    enable_testing()

    add_test_exe(test_array tests/test_array.cpp 1)
    add_test_exe(test_string tests/test_string.cpp 1)
    add_test_exe(test_arena tests/test_arena.cpp 1)

    add_test_exe(bench_string tests/benchmarks/bench_string.cpp 1)
    add_test_exe(bench_string_header tests/benchmarks/bench_string_header.cpp 1)
    add_test_exe(bench_std_headers tests/benchmarks/bench_std_headers.cpp 0)

    add_test(NAME test_array COMMAND test_array)
    add_test(NAME test_string COMMAND test_string)
    add_test(NAME test_arena COMMAND test_arena)

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
