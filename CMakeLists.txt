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

find_package(Catch2 3 REQUIRED)

enable_testing()

add_executable(test_seq tests/test_seq.cpp)
target_include_directories(test_seq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_seq PRIVATE Catch2::Catch2WithMain cxb)

add_executable(test_string tests/test_string.cpp)
target_include_directories(test_string PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_string PRIVATE Catch2::Catch2WithMain cxb)

add_test(NAME seq_tests COMMAND test_seq)
add_test(NAME string_tests COMMAND test_string)

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

    add_test(NAME c_api_c_tests COMMAND test_c_api_c)
    add_test(NAME c_api_cpp_tests COMMAND test_c_api_cpp)
endif()
