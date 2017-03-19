cmake_minimum_required(VERSION 2.8)
project(monolog CXX)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -pedantic -Wextra -Wno-missing-field-initializers")
enable_language(C)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Threads REQUIRED)
find_package(Thrift)
include(CTest)

# Pre-requisites
add_subdirectory(external/gtest-1.7.0)

# Monolog library
add_subdirectory(libmonolog)

# Datastore
add_subdirectory(libds)

# Applications
add_subdirectory(app)
