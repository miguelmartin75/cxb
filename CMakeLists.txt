cmake_minimum_required(VERSION 3.5)

project(cxb LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

set(CXB_SRCS "cxb.cpp")

# Add compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-ftime-trace)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC doesn't support -ftime-trace, but we can add other GCC-specific flags if needed
    add_compile_options(-Wall -Wextra)
endif()

add_library(cxb SHARED ${CXB_SRCS})

find_package(Catch2 3 REQUIRED)

enable_testing()

add_executable(test_seq tests/test_seq.cpp)
target_include_directories(test_seq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_seq PRIVATE Catch2::Catch2WithMain cxb)

add_test(NAME seq_tests COMMAND test_seq)
