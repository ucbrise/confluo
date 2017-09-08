cmake_minimum_required(VERSION 2.8)
project(libtimeseries CXX)

find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(TIMESERIES_INCLUDE timeseries)

include_directories(${gtest_SOURCE_DIR}/include
  ${MONOLOG_INCLUDE}
  ${DATASTORE_INCLUDE}
  ${UTILS_INCLUDE}
  ${ATOMIC_INCLUDE}
  ${TIMESERIES_INCLUDE}
  ${THRIFT_INCLUDE})

# Build server executable
file(GLOB_RECURSE server_sources src/server/*.cc)
add_executable(tsserver ${server_sources})
target_link_libraries(tsserver ${THRIFT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Build client library
set(client_sources src/server/timeseries_db_constants.cc 
  src/server/timeseries_db_types.cc
  src/server/timeseries_db_service.cc)
add_library(tsclient STATIC ${client_sources} ${CMAKE_THREAD_LIBS_INIT})

# Build perf
file(GLOB_RECURSE perf_sources perf/*.cc)
add_executable(tssperf ${perf_sources})
target_link_libraries(tssperf tsclient ${CMAKE_THREAD_LIBS_INIT} ${THRIFT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Build test
file(GLOB_RECURSE test_sources test/*.cc)
add_executable(tstest ${test_sources})
target_link_libraries(tstest gs gtest_main ${CMAKE_THREAD_LIBS_INIT})

# Register test
enable_testing()
add_test(TimeSeriesTest ${EXECUTABLE_OUTPUT_PATH}/tstest)
