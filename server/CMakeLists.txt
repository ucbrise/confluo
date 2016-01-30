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
set(MICA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../mica/include)

file(GLOB_RECURSE SERVER_SOURCES ${SOURCES_DIR}/*.cc)
set(SERVER_CLIENT_SOURCES ${SOURCES_DIR}/Server.cc)

include_directories(${INCLUDE_DIR} ${CORE_INCLUDE_DIR} ${MICA_INCLUDE_DIR})

add_executable(server ${SERVER_SOURCES})
add_executable(mica_server ${SERVER_SOURCES})

set_target_properties(mica_server PROPERTIES COMPILE_FLAGS "-DMICA_SERVER")

add_library(serverclient ${SERVER_CLIENT_SOURCES})

target_link_libraries(server logstore ${THRIFT_LIBRARIES})
target_link_libraries(mica_server mica ${THRIFT_LIBRARIES})