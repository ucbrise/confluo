set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(RPC_INCLUDE rpc)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)
set(DIALOG_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libdialog/dialog)
set(THRIFT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../external/thrift-0.10.0/lib/cpp/src)
set(THRIFT_BUILD_INCLUDE ${PROJECT_BINARY_DIR}/external/thrift-0.10.0)

include_directories(${gtest_SOURCE_DIR}/include
  ${RPC_INCLUDE}
  ${UTILS_INCLUDE}
  ${DIALOG_INCLUDE}
  ${THRIFT_INCLUDE}
  ${THRIFT_BUILD_INCLUDE})
  
# Build server executable
set(SERVER_SOURCES src/dialog_constants.cc
                   src/dialog_service.cc 
                   src/dialog_server.cc 
                   src/dialog_types.cc)
add_executable(dialogd ${SERVER_SOURCES})
LINK_AGAINST_THRIFT_LIBRARY(dialogd thrift)

set(CLIENT_SOURCES src/dialog_constants.cc 
                   src/dialog_service.cc 
                   src/dialog_types.cc)
add_library(dclient STATIC ${CLIENT_SOURCES})
LINK_AGAINST_THRIFT_LIBRARY(dclient thrift)

# Build test
file(GLOB_RECURSE TEST_SOURCES test/*.cc)
add_executable(rpctest ${TEST_SOURCES})
target_link_libraries(rpctest gtest_main dclient)
LINK_AGAINST_THRIFT_LIBRARY(rpctest thrift)

# install
install(DIRECTORY rpc/
  DESTINATION include
  FILES_MATCHING PATTERN "*")

install(TARGETS dialogd dclient
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

# Register test
enable_testing()
add_test(RpcTest ${EXECUTABLE_OUTPUT_PATH}/rpctest)


