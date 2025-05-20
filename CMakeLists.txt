cmake_minimum_required(VERSION 3.5)

project(cxb LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

set(CXB_SRCS "cxb.h")

find_package(Catch2 3 REQUIRED)
add_executable(test_seq tests/test_seq.cpp)
target_include_directories(test_seq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_seq PRIVATE Catch2::Catch2WithMain)
