cmake_minimum_required(VERSION 2.8)
project(ls-test CXX)

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

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
set(TESTS_PATH ${EXECUTABLE_OUTPUT_PATH} PARENT_SCOPE)

set(INCLUDE ../core/include)

include_directories(${gtest_SOURCE_DIR}/include
    ${INCLUDE})

file(GLOB_RECURSE test_sources src/*.cc)
add_executable(ls_test ${test_sources})
target_link_libraries(ls_test gtest_main)
