set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(CONFLUO_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/confluo)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)

include_directories(${GTEST_INCLUDE_DIR}
  ${CONFLUO_INCLUDE}
  ${UTILS_INCLUDE}
  ${Boost_INCLUDE_DIRS})

if (BUILD_TESTS)
  # Build test
  file(GLOB_RECURSE test_sources test/*.cc)
  add_executable(ctest ${test_sources})
  target_link_libraries(ctest ${TEST_LINK_LIBS} ${lz4_STATIC_LIB})
  add_dependencies(ctest googletest lz4)

  # Register test
  enable_testing()
  add_test(ConfluoTest ${EXECUTABLE_OUTPUT_PATH}/ctest)
endif ()

# install
install(DIRECTORY confluo/
  DESTINATION include
  FILES_MATCHING PATTERN "*")
