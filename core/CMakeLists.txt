cmake_minimum_required(VERSION 2.8)
project(log-store-core CXX)

find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -B /usr/local/share/libhugetlbfs")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})

set(INCLUDE include)
file(GLOB_RECURSE SOURCE_FILES src/*.cc)
include_directories(${INCLUDE})
add_library(logstore STATIC ${SOURCE_FILES})
target_link_libraries(logstore ${CMAKE_THREAD_LIBS_INIT})
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code: has to have -mt
  target_link_libraries(logstore boost_thread-mt boost_system-mt)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  # On EC2, can't have -mt
  target_link_libraries(logstore boost_thread boost_system)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set_target_properties(logstore PROPERTIES LINKER_LANGUAGE CXX)
