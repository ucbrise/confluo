set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(DIALOG_INCLUDE dialog)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)
set(EXAMPLES_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../examples/libtimeseries)
set(TEST_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../examples/libtimeseries/test)

include_directories(${GTEST_INCLUDE_DIR}
  ${DIALOG_INCLUDE}
  ${UTILS_INCLUDE}
  ${EXAMPLES_INCLUDE}
  ${TEST_INCLUDE}
  ${Boost_INCLUDE_DIRS})

if (BUILD_TESTS)
  # Build test
  file(GLOB_RECURSE test_sources test/*.cc)
  add_executable(dtest ${test_sources})
  target_link_libraries(dtest ${TEST_LINK_LIBS})
  add_dependencies(dtest googletest)
  
  # Register test
  enable_testing()
  add_test(DiaLogTest ${EXECUTABLE_OUTPUT_PATH}/dtest)
endif ()

# install
install(DIRECTORY dialog/
  DESTINATION include
  FILES_MATCHING PATTERN "*")
