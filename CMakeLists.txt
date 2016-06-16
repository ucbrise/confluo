cmake_minimum_required(VERSION 2.8)
project(log-store CXX)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -Wno-format")
enable_language(C)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)

find_package(Threads REQUIRED)
set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.53 REQUIRED COMPONENTS thread system)
find_package(Gperftools REQUIRED)
find_package(JeMalloc REQUIRED)

# Builds all sub-modules
#add_subdirectory(server)
#add_subdirectory(shell)
add_subdirectory(bench)

# Testing framework
# Build google testing framework
add_subdirectory(external/gtest-1.7.0)

# Build and run all tests for succinct
add_subdirectory(test)

enable_testing()
add_test(LSTest ${TESTS_PATH}/ls_test)
