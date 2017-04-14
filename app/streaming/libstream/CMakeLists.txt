cmake_minimum_required(VERSION 2.8)
project(libstream CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(TIMESERIES_INCLUDE streaming)

include_directories(${gtest_SOURCE_DIR}/include
  ${MONOLOG_INCLUDE}
  ${DATASTORE_INCLUDE}
  ${UTILS_INCLUDE}
  ${ATOMIC_INCLUDE}
  ${TIMESERIES_INCLUDE}
  ${THRIFT_INCLUDE})

# Build server executable
file(GLOB_RECURSE server_sources src/server/*.cc)
add_executable(sserver ${server_sources})
target_link_libraries(sserver ${THRIFT_LIBRARIES})

# Build client library
set(client_sources src/server/stream_service_constants.cc 
  src/server/stream_service_types.cc
  src/server/stream_service.cc)
add_library(ssclient STATIC ${client_sources})

# Build perf
add_executable(pperf perf/producer_benchmark.cc)
target_link_libraries(pperf ssclient ${CMAKE_THREAD_LIBS_INIT} ${THRIFT_LIBRARIES})
add_executable(cperf perf/consumer_benchmark.cc)
target_link_libraries(cperf ssclient ${CMAKE_THREAD_LIBS_INIT} ${THRIFT_LIBRARIES})

# Build test
file(GLOB_RECURSE test_sources test/*.cc)
add_executable(stest ${test_sources})
target_link_libraries(stest gs gtest_main)

# Register test
enable_testing()
add_test(StreamingTest ${EXECUTABLE_OUTPUT_PATH}/stest)