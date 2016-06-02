cmake_minimum_required(VERSION 2.8)
project(log-store-core CXX)

find_package(Threads)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})

set(INCLUDE include)
file(GLOB_RECURSE SOURCE_FILES src/*.cc)
include_directories(${INCLUDE})

add_library(logstore STATIC ${SOURCE_FILES})

target_link_libraries(logstore ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(logstore PROPERTIES LINKER_LANGUAGE CXX)
