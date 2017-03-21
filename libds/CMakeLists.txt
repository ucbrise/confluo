cmake_minimum_required(VERSION 2.8)
project(libmonolog CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(DATASTORE_INCLUDE datastore)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)
set(ATOMIC_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libatomic/atomic)

include_directories(${gtest_SOURCE_DIR}/include
  ${DATASTORE_INCLUDE}
  ${UTILS_INCLUDE}
  ${ATOMIC_INCLUDE})

# Build perf
file(GLOB_RECURSE perf_sources perf/*.cc)
add_executable(tsperf ${perf_sources})
target_link_libraries(tsperf ${CMAKE_THREAD_LIBS_INIT})

## Build test
#file(GLOB_RECURSE test_sources test/*.cc)
#add_executable(gstest ${test_sources})
#target_link_libraries(gstest gs gtest_main)

## Register test
#enable_testing()
#add_test(GraphStoreTest ${EXECUTABLE_OUTPUT_PATH}/gstest)
