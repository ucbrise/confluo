cmake_minimum_required(VERSION 2.8)
project(libls-perf CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE ../logstore)

include_directories(${gtest_SOURCE_DIR}/include ${INCLUDE} ${MONOLOG_INCLUDE})

file(GLOB_RECURSE perf_sources *.cc)
add_executable(lsperf ${perf_sources})
target_link_libraries(lsperf gtest_main)
