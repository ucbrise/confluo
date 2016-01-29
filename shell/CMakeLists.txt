cmake_minimum_required(VERSION 2.8)
project(log-store-shell CXX)

find_package(Thrift REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include)
set(SERVER_INCLUDE ../server/include)
include_directories(${INCLUDE} ${SERVER_INCLUDE})
add_executable(shell src/shell.cc)

target_link_libraries(shell serverclient ${THRIFT_LIBRARIES})
