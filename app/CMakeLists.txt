cmake_minimum_required(VERSION 2.8)
project(monolog-apps CXX)

set(MONOLOG_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libmonolog/monolog)
set(DATASTORE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libds/datastore)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)
set(ATOMIC_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libatomic/atomic)

# Add all applications
add_subdirectory(graphstore)
add_subdirectory(timeseries)
add_subdirectory(streaming)
