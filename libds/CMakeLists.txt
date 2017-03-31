cmake_minimum_required(VERSION 2.8)
project(libmonolog CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(DATASTORE_INCLUDE datastore)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)
set(ATOMIC_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libatomic/atomic)
set(MONOLOG_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libmonolog/monolog)

include_directories(${gtest_SOURCE_DIR}/include
  ${DATASTORE_INCLUDE}
  ${UTILS_INCLUDE}
  ${ATOMIC_INCLUDE}
  ${MONOLOG_INCLUDE}
  ${THRIFT_INCLUDE})
  
# Build server executable
file(GLOB_RECURSE server_sources src/server/*.cc)
add_executable(lsserver ${server_sources})
target_link_libraries(lsserver ${THRIFT_LIBRARIES})

set(client_sources src/server/log_store_constants.cc 
  src/server/log_store_types.cc
  src/server/log_store_service.cc)
add_library(lsclient STATIC ${client_sources})

file(GLOB_RECURSE coord_sources src/coordinator/*.cc)
set(coord_sources ${coord_sources})
add_executable(lscoordinator ${coord_sources})
target_link_libraries(lscoordinator lsclient ${CMAKE_THREAD_LIBS_INIT} ${THRIFT_LIBRARIES})

# Build perf
file(GLOB_RECURSE cc_perf_sources perf/concurrency_control_perf.cc)
add_executable(ccperf ${cc_perf_sources})
target_link_libraries(ccperf ${CMAKE_THREAD_LIBS_INIT})

file(GLOB_RECURSE lss_perf_sources perf/log_store_server_perf.cc)
add_executable(lssperf ${lss_perf_sources})
target_link_libraries(lssperf lsclient ${CMAKE_THREAD_LIBS_INIT} ${THRIFT_LIBRARIES})

# Build test
file(GLOB_RECURSE test_sources test/*.cc)
add_executable(dstest ${test_sources})
target_link_libraries(dstest gtest_main)

# Register test
enable_testing()
add_test(DataStoreTest ${EXECUTABLE_OUTPUT_PATH}/dstest)
