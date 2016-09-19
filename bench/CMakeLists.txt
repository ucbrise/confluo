cmake_minimum_required(VERSION 2.8)
project(log-store-bench CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include)
set(CORE_INCLUDE ../core/include)
include_directories(${INCLUDE} ${CORE_INCLUDE})

add_executable(pktload src/packet_loader.cc)
add_executable(storage src/storage_footprint.cc)

target_link_libraries(pktload ${CMAKE_THREAD_LIBS_INIT} ${JEMALLOC_LIBRARIES})