cmake_minimum_required(VERSION 2.8)
project(libls CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(LS_INCLUDE logstore)

include_directories(${gtest_SOURCE_DIR}/include 
  ${ATOMIC_INCLUDE} 
  ${UTILS_INCLUDE}
  ${MONOLOG_INCLUDE} 
  ${LS_INCLUDE})

file(GLOB_RECURSE test_sources test/*.cc)
add_executable(lstest ${test_sources})
target_link_libraries(lstest gtest_main)

file(GLOB_RECURSE perf_sources perf/*.cc)
add_executable(lsperf ${perf_sources})
target_link_libraries(lsperf gtest_main)

# Run all tests
enable_testing()
add_test(LogStoreTest ${EXECUTABLE_OUTPUT_PATH}/lstest)
