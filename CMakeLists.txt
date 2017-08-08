cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(dialog VERSION 0.1.0 LANGUAGES CXX C)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -pedantic -Wextra -Wno-missing-field-initializers")
enable_language(C)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif ()

set(CMAKE_VERBOSE_MAKEFILE false
    CACHE STRING "Show all outputs including compiler lines." FORCE)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Threads REQUIRED)
include(CTest)

# Pre-requisites
add_subdirectory(external/gtest-1.7.0)

# DiaLog core
add_subdirectory(libdialog)

# RPC
find_package(Thrift)
if(THRIFT_FOUND)
  message(STATUS "Building RPC")
  add_subdirectory(librpc)
else()
  message(WARNING "Apache Thrift not found, skipping rpc build")
endif()
