cmake_minimum_required(VERSION 2.8)
project(monolog-apps CXX)

set(MONOLOG_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libmonolog/monolog)

# Add all applications
add_subdirectory(logstore)
