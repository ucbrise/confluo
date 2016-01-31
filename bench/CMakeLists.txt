cmake_minimum_required(VERSION 2.8)
project(log-store-bench CXX)

find_package(Thrift REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include)
set(SERVER_INCLUDE ../server/include)
set(CORE_INCLUDE ../core/include)
set(MICA_INCLUDE ../mica/include)
include_directories(${INCLUDE} ${SERVER_INCLUDE} ${CORE_INCLUDE} ${MICA_INCLUDE})
add_executable(lsbench src/log_store_benchmark.cc)
add_executable(mbench src/micro_benchmark.cc)
add_executable(mmbench src/micro_benchmark.cc)

set_target_properties(mmbench PROPERTIES COMPILE_FLAGS "-DMICA_BENCHMARK")

target_link_libraries(lsbench serverclient ${THRIFT_LIBRARIES})
target_link_libraries(mbench logstore)
target_link_libraries(mmbench mica)
