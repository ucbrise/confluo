cmake_minimum_required(VERSION 2.8)
project(libmonolog CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(GRAPHSTORE_INCLUDE graphstore)

include_directories(${gtest_SOURCE_DIR}/include
  ${MONOLOG_INCLUDE}
  ${DATASTORE_INCLUDE}
  ${UTILS_INCLUDE}
  ${GRAPHSTORE_INCLUDE}
  ${THRIFT_INCLUDE})

# Build static library
add_library(gs STATIC src/graph_store.cc)

# Build server executable
file(GLOB_RECURSE server_sources src/server/*.cc)
add_executable(gsserver ${server_sources})
target_link_libraries(gsserver gs ${THRIFT_LIBRARIES})

# Build perf
file(GLOB_RECURSE perf_sources perf/*.cc)
add_executable(gsperf ${perf_sources})
target_link_libraries(gsperf gs ${CMAKE_THREAD_LIBS_INIT})

# Build test
file(GLOB_RECURSE test_sources test/*.cc)
add_executable(gstest ${test_sources})
target_link_libraries(gstest gs gtest_main)

# Register test
enable_testing()
add_test(GraphStoreTest ${EXECUTABLE_OUTPUT_PATH}/gstest)
