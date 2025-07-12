cmake_minimum_required(VERSION 3.5)

project(cxb LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

# Options
option(BUILD_C_API_TESTS "Build C API compatibility tests" ON)

set(CXB_SRCS "cxb/cxb.cpp")

# Add compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-ftime-trace -Wall -Wextra)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra)
endif()

add_library(cxb SHARED ${CXB_SRCS})
target_include_directories(cxb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(deps/Catch2)

enable_testing()

function(add_test_exe test_name test_source fsanitize)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${test_name} PRIVATE Catch2::Catch2WithMain cxb ${ARGN})
    if(${fsanitize} AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
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

add_test_exe(test_array tests/test_array.cpp YES)
add_test_exe(test_string tests/test_string.cpp YES)
add_test_exe(bench_string benchmarks/bench_string.cpp YES)
add_test_exe(bench_std_headers benchmarks/bench_std_headers.cpp NO)
add_test_exe(bench_string_header benchmarks/bench_string_header.cpp YES)

add_test(NAME test_array COMMAND test_array)
add_test(NAME test_string COMMAND test_string)

# C API tests (conditional)
if(BUILD_C_API_TESTS)
    # Pure C test
    add_executable(test_c_api_c tests/test_c_api_c.c tests/c_api_test.cpp)
    target_include_directories(test_c_api_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_c_api_c PRIVATE cxb)

    # C++ test that uses Catch2
    add_executable(test_c_api_cpp tests/test_c_api_cpp.cpp tests/c_api_test.cpp)
    target_include_directories(test_c_api_cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_c_api_cpp PRIVATE Catch2::Catch2WithMain cxb)

    add_test(NAME test_c_api_c COMMAND test_c_api_c)
    add_test(NAME test_c_api_cpp COMMAND test_c_api_cpp)
endif()
