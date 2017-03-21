cmake_minimum_required(VERSION 2.8)
project(libmonolog CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(ATOMIC_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libatomic/atomic)
set(INCLUDE monolog)
include_directories(${gtest_SOURCE_DIR}/include ${ATOMIC_INCLUDE} ${INCLUDE})

file(GLOB_RECURSE test_sources test/*.cc)
add_executable(mtest ${test_sources})
target_link_libraries(mtest gtest_main)

file(GLOB_RECURSE perf_sources perf/*.cc)
add_executable(mperf ${perf_sources})
target_link_libraries(mperf gtest_main)

# Run all tests
enable_testing()
add_test(MonoLogTest ${EXECUTABLE_OUTPUT_PATH}/mtest)
