set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(RPC_INCLUDE rpc)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)
set(CONFLUO_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libconfluo/confluo)

include_directories(${gtest_SOURCE_DIR}/include
  ${RPC_INCLUDE}
  ${UTILS_INCLUDE}
  ${CONFLUO_INCLUDE}
  ${THRIFT_INCLUDE}
  ${THRIFT_BUILD_INCLUDE}
  ${Boost_INCLUDE_DIR})
  
# Build server executable
set(SERVER_SOURCES src/rpc_constants.cc
                   src/rpc_service.cc 
                   src/rpc_server.cc 
                   src/rpc_types.cc)
add_executable(confluod ${SERVER_SOURCES})
target_link_libraries(confluod ${CMAKE_THREAD_LIBS_INIT} thriftstatic ${lz4_STATIC_LIB})
add_dependencies(confluod thrift lz4)

set(CLIENT_SOURCES src/rpc_constants.cc 
                   src/rpc_service.cc 
                   src/rpc_types.cc)
add_library(rpcclient STATIC ${CLIENT_SOURCES})
target_link_libraries(rpcclient thriftstatic)
add_dependencies(rpcclient thrift)

if (BUILD_TESTS)
  # Build test
  file(GLOB_RECURSE TEST_SOURCES test/*.cc)
  add_executable(rpctest ${TEST_SOURCES})
  target_link_libraries(rpctest ${TEST_LINK_LIBS} rpcclient thriftstatic)
  add_dependencies(rpctest googletest thrift)
  
  # Register test
  enable_testing()
  add_test(RpcTest ${EXECUTABLE_OUTPUT_PATH}/rpctest)
endif()

# install
install(DIRECTORY rpc/
  DESTINATION include
  FILES_MATCHING PATTERN "*")

install(TARGETS confluod rpcclient
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)
