# CMake generated Testfile for 
# Source directory: /home/runner/work/cxb/cxb
# Build directory: /home/runner/work/cxb/cxb/build/coverage
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[test_array]=] "/home/runner/work/cxb/cxb/build/coverage/test_array")
set_tests_properties([=[test_array]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/cxb/cxb/CMakeLists.txt;102;add_test;/home/runner/work/cxb/cxb/CMakeLists.txt;0;")
add_test([=[test_string]=] "/home/runner/work/cxb/cxb/build/coverage/test_string")
set_tests_properties([=[test_string]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/cxb/cxb/CMakeLists.txt;103;add_test;/home/runner/work/cxb/cxb/CMakeLists.txt;0;")
add_test([=[test_arena]=] "/home/runner/work/cxb/cxb/build/coverage/test_arena")
set_tests_properties([=[test_arena]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/cxb/cxb/CMakeLists.txt;104;add_test;/home/runner/work/cxb/cxb/CMakeLists.txt;0;")
add_test([=[test_hm]=] "/home/runner/work/cxb/cxb/build/coverage/test_hm")
set_tests_properties([=[test_hm]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/cxb/cxb/CMakeLists.txt;105;add_test;/home/runner/work/cxb/cxb/CMakeLists.txt;0;")
add_test([=[test_algos]=] "/home/runner/work/cxb/cxb/build/coverage/test_algos")
set_tests_properties([=[test_algos]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/cxb/cxb/CMakeLists.txt;106;add_test;/home/runner/work/cxb/cxb/CMakeLists.txt;0;")
subdirs("deps/Catch2")
