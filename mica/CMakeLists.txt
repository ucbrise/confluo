cmake_minimum_required(VERSION 2.8)
project(mica-store-core CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})

set(INCLUDE include)
file(GLOB_RECURSE SOURCE_FILES src/*.cc)
include_directories(${INCLUDE} ${OPENSSL_INCLUDE_DIR})

add_library(mica STATIC ${SOURCE_FILES} ${OPENSSL_LIBRARIES})
target_link_libraries(mica ${CMAKE_THREAD_LIBS_INIT} crypto m)
set_target_properties(mica PROPERTIES LINKER_LANGUAGE CXX)
