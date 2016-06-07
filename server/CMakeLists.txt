cmake_minimum_required(VERSION 2.8)
project(log-store-server CXX)

find_package(Thrift REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core/include)

file(GLOB_RECURSE SERVER_SOURCES ${SOURCES_DIR}/*.cc)
set(SERVER_CLIENT_SOURCES ${SOURCES_DIR}/Server.cc)

include_directories(${INCLUDE_DIR} ${CORE_INCLUDE_DIR})

add_executable(server ${SERVER_SOURCES})

add_library(serverclient ${SERVER_CLIENT_SOURCES})

target_link_libraries(server ${THRIFT_LIBRARIES})
