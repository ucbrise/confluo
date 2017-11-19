set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(CONFLUO_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../libconfluo/confluo)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../libutils/utils)
set(TIMESERIES_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/timeseries)

include_directories(${GTEST_INCLUDE_DIR}
  ${CONFLUO_INCLUDE}
  ${UTILS_INCLUDE}
  ${TIMESERIES_INCLUDE}
  ${Boost_INCLUDE_DIRS})

if (BUILD_TESTS)
  # Build test
  file(GLOB_RECURSE test_sources test/*.cc)
  add_executable(tstest ${test_sources})
  target_link_libraries(tstest ${TEST_LINK_LIBS})
  add_dependencies(tstest googletest)
  
  # Register test
  enable_testing()
  add_test(TimeseriesTest ${EXECUTABLE_OUTPUT_PATH}/tstest)
endif ()
