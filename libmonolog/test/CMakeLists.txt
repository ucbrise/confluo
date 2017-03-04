cmake_minimum_required(VERSION 2.8)
project(monolog-test CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE ../monolog)

include_directories(${gtest_SOURCE_DIR}/include ${INCLUDE})

file(GLOB_RECURSE test_sources src/*.cc)
add_executable(mtest ${test_sources})
target_link_libraries(mtest gtest_main)

# Run all tests
enable_testing()
add_test(MonoLogTest ${EXECUTABLE_OUTPUT_PATH}/mtest)
