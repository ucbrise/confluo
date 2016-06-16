cmake_minimum_required(VERSION 2.8)
project(ls-test CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
set(TESTS_PATH ${EXECUTABLE_OUTPUT_PATH} PARENT_SCOPE)

set(INCLUDE ../core/include)

include_directories(${gtest_SOURCE_DIR}/include ${INCLUDE})

file(GLOB_RECURSE test_sources src/*.cc)
add_executable(ls_test ${test_sources})
target_link_libraries(ls_test gtest_main)
