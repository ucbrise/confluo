set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

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
add_executable(confluod
        rpc/rpc_type_conversions.h
        rpc/rpc_client.h
        rpc/rpc_configuration_params.h
        rpc/rpc_alert_stream.h
        rpc/rpc_record_stream.h
        rpc/rpc_server.h
        rpc/rpc_constants.h
        rpc/rpc_types.tcc
        rpc/rpc_service.h
        rpc/rpc_record_batch_builder.h
        rpc/rpc_defaults.h
        rpc/rpc_types.h
        rpc/rpc_service.tcc
        src/rpc_constants.cc
        src/rpc_service.cc
        src/rpc_server.cc
        src/rpc_types.cc
        src/rpc_alert_stream.cc
        src/rpc_client.cc
        src/rpc_record_batch_builder.cc
        src/rpc_record_stream.cc
        src/rpc_type_conversions.cc
        src/confluo_server.cc)
target_link_libraries(confluod confluo thriftstatic)
add_dependencies(confluod thrift)

add_library(rpcclient STATIC
        rpc/rpc_constants.h
        src/rpc_constants.cc
        rpc/rpc_server.h
        src/rpc_server.cc
        rpc/rpc_service.h
        rpc/rpc_service.tcc
        src/rpc_service.cc
        rpc/rpc_types.h
        src/rpc_types.cc
        rpc/rpc_client.h
        src/rpc_client.cc
        rpc/rpc_alert_stream.h
        src/rpc_alert_stream.cc
        rpc/rpc_record_batch_builder.h
        src/rpc_record_batch_builder.cc
        rpc/rpc_type_conversions.h
        src/rpc_type_conversions.cc
        rpc/rpc_record_stream.h
        src/rpc_record_stream.cc
        rpc/rpc_configuration_params.h
        rpc/rpc_defaults.h)
target_link_libraries(rpcclient thriftstatic)
add_dependencies(rpcclient thrift)

if (BUILD_TESTS)
  # Build test
  add_executable(rpctest
          rpc/rpc_type_conversions.h
          rpc/rpc_client.h
          rpc/rpc_configuration_params.h
          rpc/rpc_alert_stream.h
          rpc/rpc_record_stream.h
          rpc/rpc_server.h
          rpc/rpc_constants.h
          rpc/rpc_types.tcc
          rpc/rpc_service.h
          rpc/rpc_record_batch_builder.h
          rpc/rpc_defaults.h
          rpc/rpc_types.h
          rpc/rpc_service.tcc
          test/client_read_ops_test.h
          test/test_main.cc
          test/client_connection_test.h
          test/rpc_test_utils.h
          test/client_write_ops_test.h)
  target_link_libraries(rpctest confluo gtest gtest_main rpcclient)
  add_dependencies(rpctest googletest)

  # Register test
  enable_testing()
  add_test(RpcTest rpctest)
endif ()

# install
install(DIRECTORY rpc/
        DESTINATION include
        FILES_MATCHING PATTERN "*")

install(TARGETS confluod rpcclient
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
