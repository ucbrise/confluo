set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(DIALOG_INCLUDE dialog)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)

include_directories(${gtest_SOURCE_DIR}/include
  ${DIALOG_INCLUDE}
  ${UTILS_INCLUDE})

# Build test
file(GLOB_RECURSE test_sources test/*.cc)
add_executable(dtest ${test_sources})
target_link_libraries(dtest gtest_main)

# install
install(DIRECTORY dialog/
  DESTINATION include
  FILES_MATCHING PATTERN "*")

# Register test
enable_testing()
add_test(DiaLogTest ${EXECUTABLE_OUTPUT_PATH}/dtest)
