set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(CONFLUO_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../libconfluo/confluo)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../libutils/utils)
set(RPC_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../librpc/rpc)
set(STREAM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/streaming)

include_directories(${GTEST_INCLUDE_DIR}
        ${CONFLUO_INCLUDE}
        ${UTILS_INCLUDE}
        ${RPC_INCLUDE}
        ${STREAM_INCLUDE}
        ${Boost_INCLUDE_DIRS})

if (BUILD_TESTS)
  # Build test
  add_executable(stest
          streaming/stream_producer.h
          streaming/stream_consumer.h
          test/streaming_test_utils.h
          test/test_main.cc
          test/stream_test.h)
  target_link_libraries(stest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT} rpcclient confluo thriftstatic ${lz4_STATIC_LIB})
  add_dependencies(stest googletest rpctest thrift lz4)

  # Register test
  enable_testing()
  add_test(StreamTest ${EXECUTABLE_OUTPUT_PATH}/stest)
endif ()
